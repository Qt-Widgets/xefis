/* vim:ts=4
 *
 * Copyleft 2012…2016  Michał Gawron
 * Marduk Unix Labs, http://mulabs.org/
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Visit http://www.gnu.org/licenses/gpl-3.0.html for more information on licensing.
 */

// Standard:
#include <cstddef>
#include <memory>
#include <ctime>

// Lib:
#include <boost/lexical_cast.hpp>
#include <boost/optional.hpp>

// Xefis:
#include <xefis/config/all.h>
#include <xefis/core/stdexcept.h>
#include <xefis/core/module_manager.h>
#include <xefis/core/system.h>
#include <xefis/core/xefis.h>
#include <xefis/support/bus/serial_port.h>
#include <xefis/support/protocols/nmea/parser.h>
#include <xefis/support/protocols/nmea/mtk.h>
#include <xefis/utility/qdom.h>
#include <xefis/utility/resource.h>
#include <xefis/utility/mutex.h>
#include <xefis/utility/time_helper.h>

// Local:
#include "gps.h"


XEFIS_REGISTER_MODULE_CLASS ("io/gps", GPS)


#define FUZZ_INPUT

using namespace xf::exception_ops;


constexpr unsigned int	GPS::kConnectionAttemptsPerPowerCycle;
constexpr Time			GPS::kPowerRestartDelay;
constexpr Time			GPS::kAliveCheckInterval;
constexpr unsigned int	GPS::kMaxRestartAttempts;
constexpr const char*	GPS::MTK_SET_NMEA_BAUDRATE;
constexpr const char*	GPS::MTK_SET_NMEA_FREQUENCIES;
constexpr const char*	GPS::MTK_SET_NMEA_POSITION_FIX_INTERVAL;


GPS::Connection::Connection (GPS& gps_module, PowerCycle& power_cycle, unsigned int baud_rate):
	_gps_module (gps_module),
	_power_cycle (power_cycle),
	// Store requested baud rate separately, since _serial_port_config may change it
	// (align to nearest allowed baud rate):
	_requested_physical_baud_rate (baud_rate)
{
	_gps_module.log() << "Create GPS serial connection" << std::endl;

	_alive_check_timer = std::make_unique<QTimer> (this);
	_alive_check_timer->setInterval (kAliveCheckInterval.quantity<Millisecond>());
	_alive_check_timer->setSingleShot (true);
	QObject::connect (_alive_check_timer.get(), &QTimer::timeout, this, &Connection::alive_check_failed);

	_serial_port_config.set_device_path (_gps_module._device_path);
	_serial_port_config.set_baud_rate (baud_rate);
	_serial_port_config.set_data_bits (8);
	_serial_port_config.set_stop_bits (1);
	_serial_port_config.set_parity_bit (xf::SerialPort::Parity::None);
	_serial_port_config.set_hardware_flow_control (false);

	_serial_port = _gps_module.module_manager()->xefis()->system()->allocate_serial_port (std::bind (&GPS::Connection::serial_data_ready, this),
																						  std::bind (&GPS::Connection::serial_failure, this));
	_serial_port->set_max_read_failures (3);
	_serial_port->set_logger (_gps_module.log());

	open_device();
}


GPS::Connection::~Connection()
{
	_gps_module.log() << "Stop GPS serial connection" << std::endl;
	_gps_module.reset_data_properties();
	_gps_module._serviceable.write (false);
	_reliable_fix_quality = false;
}


void
GPS::Connection::data_updated()
{
	if (_serial_port && !_serial_port->flushed())
		_serial_port->flush_async();
}


unsigned int
GPS::Connection::requested_physical_baud_rate() const
{
	return _requested_physical_baud_rate;
}


void
GPS::Connection::open_device()
{
	_serial_port->set_configuration (_serial_port_config);

	bool has_thrown = xf::Exception::guard ([&] {
		_alive_check_timer->start();

		_gps_module.log() << "Opening device " << _gps_module._device_path << " at " << _serial_port_config.baud_rate() << " bps" << std::endl;

		if (_serial_port->open())
			initialize_device();
		else
			failure ("couldn't open serial port");
	});

	if (has_thrown)
		failure ("exception in open_device()");
}


void
GPS::Connection::initialize_device()
{
	_nmea_parser = std::make_unique<xf::nmea::Parser> (this);

	_gps_module.log() << "Sending initialization commands." << std::endl;

	_serial_port->write (xf::nmea::make_mtk_sentence (get_nmea_frequencies_setup_messages (_serial_port_config.baud_rate())));
	// Now send user setup commands:
	for (auto s: _gps_module._boot_pmtk_commands)
	{
		std::string pmtk = xf::nmea::make_mtk_sentence (s);
		_serial_port->write (pmtk);
		// Even if all data can't be written now, it shall be flushed
		// eventually with _serial_port->flush_async() in data_updated().
	}
}


void
GPS::Connection::request_new_baud_rate (unsigned int baud_rate)
{
	_gps_module.log() << "Requesting baud-rate switch from " << _serial_port_config.baud_rate() << " to " << baud_rate << std::endl;
	std::string set_baud_rate_message = xf::nmea::make_mtk_sentence (MTK_SET_NMEA_BAUDRATE + ","_str + boost::lexical_cast<std::string> (baud_rate));
	_serial_port->write (set_baud_rate_message);
	_serial_port->flush();
	_serial_port->close();
}


void
GPS::Connection::alive_check_failed()
{
	failure ("alive check failed");
}


void
GPS::Connection::failure (std::string const& reason)
{
	_gps_module.log() << "Failure detected" << (reason.empty() ? "" : (": " + reason)) << ", closing device " << _gps_module._device_path << std::endl;
	_power_cycle.notify_connection_failure();
}


void
GPS::Connection::serial_data_ready()
{
	_nmea_parser->feed (_serial_port->input_buffer());
	_serial_port->input_buffer().clear();

	bool processed = true;
	do {
		processed = true;

		try {
			processed = _nmea_parser->process_one();

			if (processed)
				_alive_check_timer->start();
		}
		catch (...)
		{
			if (_gps_module._read_errors.configured())
				_gps_module._read_errors = *_gps_module._read_errors + 1;
			_gps_module.log() << "Exception when processing NMEA sentence: " << std::current_exception() << std::endl;
		}
	} while (processed);
}


void
GPS::Connection::serial_failure()
{
	failure ("serial communication error");
}


void
GPS::Connection::process_nmea_sentence (xf::nmea::GPGGA const& sentence)
{
	message_received();

	_gps_module._latitude = sentence.latitude;
	_gps_module._longitude = sentence.longitude;

	using xf::nmea::to_string;

	if (sentence.fix_quality)
		_gps_module._fix_quality = to_string (*sentence.fix_quality);
	else
		_gps_module._fix_quality.set_nil();

	_gps_module._tracked_satellites = optional_cast_to<decltype (_gps_module._dgps_station_id)::Type> (sentence.tracked_satellites);
	_gps_module._altitude_amsl = sentence.altitude_amsl;
	_gps_module._geoid_height = sentence.geoid_height;
	_gps_module._dgps_station_id = optional_cast_to<decltype (_gps_module._dgps_station_id)::Type> (sentence.dgps_station_id);
	// Use system time as reference:
	_gps_module._fix_system_timestamp = xf::TimeHelper::now();
	_gps_module._reliable_fix_quality = sentence.reliable_fix_quality();
}


void
GPS::Connection::process_nmea_sentence (xf::nmea::GPGSA const& sentence)
{
	message_received();

	if (sentence.fix_mode)
	{
		switch (*sentence.fix_mode)
		{
			case xf::nmea::GPSFixMode::Fix2D:
				_gps_module._fix_mode = "2D";
				break;

			case xf::nmea::GPSFixMode::Fix3D:
				_gps_module._fix_mode = "3D";
				break;

			default:
				_gps_module._fix_mode.set_nil();
		}
	}
	else
		_gps_module._fix_mode.set_nil();

	_gps_module._pdop = optional_cast_to<double> (sentence.pdop);
	_gps_module._vdop = optional_cast_to<double> (sentence.vdop);
	_gps_module._hdop = optional_cast_to<double> (sentence.hdop);

	if (sentence.hdop)
		_gps_module._lateral_stddev = _gps_module._receiver_accuracy * *sentence.hdop;
	else
		_gps_module._lateral_stddev.set_nil();

	if (sentence.vdop)
		_gps_module._vertical_stddev = _gps_module._receiver_accuracy * *sentence.vdop;
	else
		_gps_module._vertical_stddev.set_nil();

	if (sentence.hdop && sentence.vdop)
		_gps_module._position_stddev = _gps_module._receiver_accuracy * std::max (*sentence.hdop, *sentence.vdop);
	else
		_gps_module._position_stddev.set_nil();
}


void
GPS::Connection::process_nmea_sentence (xf::nmea::GPRMC const& sentence)
{
	message_received();

	// If values weren't updated by GGA message, use
	// position info from RMC:
	if (_gps_module._latitude.valid_age() > 1.5_s)
		_gps_module._latitude = sentence.latitude;
	if (_gps_module._longitude.valid_age() > 1.5_s)
		_gps_module._longitude = sentence.longitude;

	_gps_module._ground_speed = sentence.ground_speed;
	_gps_module._track_true = sentence.track_true;
	_gps_module._magnetic_declination = sentence.magnetic_variation;

	if (sentence.fix_date && sentence.fix_time)
		_gps_module._fix_gps_timestamp = to_unix_time (*sentence.fix_date, *sentence.fix_time);
	else
		_gps_module._fix_gps_timestamp.set_nil();

	if (sentence.receiver_status == xf::nmea::GPSReceiverStatus::Active)
		if (_gps_module._reliable_fix_quality)
			if (sentence.fix_date && sentence.fix_time)
				_gps_module.update_clock (*sentence.fix_date, *sentence.fix_time);
}


void
GPS::Connection::process_nmea_sentence (xf::nmea::PMTKACK const& sentence)
{
	message_received();

	std::string command_hint;
	if (sentence.command)
	{
		command_hint = xf::nmea::describe_mtk_command_by_id (*sentence.command);
		if (command_hint.empty())
			command_hint = *sentence.command;
	}

	if (sentence.result)
	{
		switch (*sentence.result)
		{
			case xf::nmea::MTKResult::InvalidCommand:
				_gps_module.log() << "Invalid command/packet: " << command_hint << std::endl;
				break;

			case xf::nmea::MTKResult::UnsupportedCommand:
				_gps_module.log() << "Unsupported command/packet: " << command_hint << std::endl;
				break;

			case xf::nmea::MTKResult::Failure:
				_gps_module.log() << "Valid command, but action failed for: " << command_hint << std::endl;
				break;

			case xf::nmea::MTKResult::Success:
				_gps_module.log() << "Command result: " << command_hint << ": OK" << std::endl;
				break;
		}
	}
	else
		_gps_module.log() << "Unrecognizable MTK ACK message (no result flag): " << sentence.contents() << std::endl;
}


std::string
GPS::Connection::get_nmea_frequencies_setup_messages (unsigned int baud_rate)
{
	// Set NMEA packet frequencies.
	// Index (name):
	//   0 - GLL
	//   1 - RMC
	//   2 - VTG
	//   3 - GGA
	//   4 - GSA
	//   5 - GSV
	//   ..
	//   18 - CHN
	// Available values:
	//   0 - disabled
	//   1…5 - output a packet every 1…5 position fixes

	// *_period arguments are 1 to 5 inclusive (output every 1 to 5 position fixes).
	auto get_required_baud_rate = [](Time fix_interval, int gga_period, int gsa_period, int rmc_period) -> unsigned int
	{
		// Required messages and their maximum lengths:
		constexpr signed int header = 6;
		constexpr signed int epilog = 5;
		constexpr signed int GGA_maxlen = header + 10 + 9 + 1 + 9 + 1 + 1 + 2 + 4 + 7 + 1 + 7 + 1 + 5 + epilog + 14;
		constexpr signed int GSA_maxlen = header + 1 + 1 + (12 * 2) + (3 * 4) + epilog + 17;
		constexpr signed int RMC_maxlen = header + 10 + 1 + 9 + 1 + 9 + 1 + 6 + 6 + 6 + 6 + 1 + 1 + epilog + 12;

		Frequency gga_per_second = 1.0 / fix_interval / gga_period;
		Frequency gsa_per_second = 1.0 / fix_interval / gsa_period;
		Frequency rmc_per_second = 1.0 / fix_interval / rmc_period;

		Frequency byte_freq = GGA_maxlen * gga_per_second + GSA_maxlen * gsa_per_second + RMC_maxlen * rmc_per_second;
		return static_cast<unsigned int> (std::ceil (8 * byte_freq.quantity<Hertz>()));
	};

	Time fix_interval = 100_ms;
	int gga_period = 1;
	int gsa_period = 1;
	int rmc_period = 1;

	// Decrease frequency of messages until they fit in the baud-rate.
	// Most important are GGA, then RMC, then GSA.
	// Start with least-important messages.
	while (get_required_baud_rate (fix_interval, gga_period, gsa_period, rmc_period) > baud_rate)
	{
		if (rmc_period < 5)
			rmc_period++;
		else if (gsa_period < 5)
			gsa_period++;
		else if (gga_period < 5)
			gga_period++;
		else
		{
			gga_period = 1;
			gsa_period = 1;
			rmc_period = 1;
			fix_interval += 100_ms;
		}
	}

	unsigned int fix_interval_rounded_to_100ms = static_cast<unsigned int> (fix_interval.quantity<Millisecond>()) / 100 * 100;
	std::string mtk_set_nmea_frequencies_body = (boost::format ("%s,0,%d,0,%d,%d,0,0,0,0,0,0,0,0,0,0,0,0,0,0") % MTK_SET_NMEA_FREQUENCIES % rmc_period % gga_period % gsa_period).str();
	std::string mtk_set_nmea_frequencies = xf::nmea::make_mtk_sentence (mtk_set_nmea_frequencies_body);
	std::string mtk_set_nmea_position_fix_interval_body = std::string (MTK_SET_NMEA_POSITION_FIX_INTERVAL) + "," + std::to_string (fix_interval_rounded_to_100ms);
	std::string mtk_set_nmea_position_fix_interval = xf::nmea::make_mtk_sentence (mtk_set_nmea_position_fix_interval_body);

	return mtk_set_nmea_frequencies + mtk_set_nmea_position_fix_interval;
}


inline void
GPS::Connection::message_received()
{
	if (_first_message_received)
		return;

	_first_message_received = true;
	_power_cycle.notify_connection_established();
}


GPS::PowerCycle::PowerCycle (GPS& gps_module):
	_gps_module (gps_module)
{
	// Turn on power to the device.
	_gps_module.log() << "GPS power on" << std::endl;
	_gps_module._power_on.write (true);

	// Connection will be created in data_updated().
}


GPS::PowerCycle::~PowerCycle()
{
	_connection.reset();
	// Turn off power to the device.
	_gps_module.log() << "GPS power off" << std::endl;
	_gps_module._power_on.write (false);
}


void
GPS::PowerCycle::data_updated()
{
	// _connection management is done here, since this method is called
	// from main Qt event loop, and not directly from the Connection object
	// itself.

	if (_restart_connection)
	{
		_restart_connection = false;
		_connection.reset();
	}

	if (!_connection)
	{
		++_connection_attempts;
		unsigned int baud_rate = (_connection_attempts % 2 == 0)
			? _gps_module._target_baud_rate
			: _gps_module._default_baud_rate;
		_connection = std::make_unique<Connection> (_gps_module, *this, baud_rate);
	}

	_connection->data_updated();
}


void
GPS::PowerCycle::notify_connection_failure()
{
	_gps_module.log() << "Serial connection failure." << std::endl;

	if (_connection_attempts >= kConnectionAttemptsPerPowerCycle)
		_gps_module.request_power_cycle();
	else
		_restart_connection = true;
}


void
GPS::PowerCycle::notify_connection_established()
{
	_gps_module.log() << "Stable connection established." << std::endl;
	_gps_module._serviceable.write (true);

	// Try to use target baud rate.
	// If power cycles goes beyond max allowed number, don't try to reconnect
	// if a working connection is established. Use what we have.
	if (_gps_module._power_cycle_attempts <= kMaxRestartAttempts)
	{
		// If connection attempts < kConnectionAttemptsPerPowerCycle and power cycles num <= kMaxRestartAttempts,
		// then aim for the target-baud-rate. Otherwise try to settle at the default baud rate.
		if ((_connection_attempts <= kConnectionAttemptsPerPowerCycle) == (_connection->requested_physical_baud_rate() != _gps_module._target_baud_rate))
		{
			_connection->request_new_baud_rate (_gps_module._target_baud_rate);
			_restart_connection = true;
		}
	}
	else
		_gps_module.log() << "Max connection attempts achieved, not retrying anymore." << std::endl;
}


GPS::GPS (xf::ModuleManager* module_manager, QDomElement const& config):
	Module (module_manager, config)
{
	parse_settings (config, {
		{ "serial.device", _device_path, true },
		{ "serial.baud-rate.default", _default_baud_rate, true },
		{ "serial.baud-rate.target", _target_baud_rate, true },
		{ "receiver-accuracy", _receiver_accuracy, true },
		{ "synchronize-system-clock", _synchronize_system_clock, false },
	});

	// TODO check logic that _target_baud_rate >= _default_baud_rate

	parse_properties (config, {
		{ "serviceable", _serviceable, true },
		{ "read-errors", _read_errors, true },
		{ "fix-quality", _fix_quality, true },
		{ "fix-mode", _fix_mode, true },
		{ "latitude", _latitude, true },
		{ "longitude", _longitude, true },
		{ "altitude-amsl", _altitude_amsl, true },
		{ "geoid-height", _geoid_height, true },
		{ "ground-speed", _ground_speed, true },
		{ "track.true", _track_true, true },
		{ "tracked-satellites", _tracked_satellites, true },
		{ "magnetic-declination", _magnetic_declination, true },
		{ "hdop", _hdop, true },
		{ "vdop", _vdop, true },
		{ "pdop", _pdop, true },
		{ "lateral.stddev", _lateral_stddev, true },
		{ "vertical.stddev", _vertical_stddev, true },
		{ "position.stddev", _position_stddev, true },
		{ "dgps-station-id", _dgps_station_id, true },
		{ "fix-system-timestamp", _fix_system_timestamp, true },
		{ "fix-gps-timestamp", _fix_gps_timestamp, true },
		{ "power-on", _power_on, false },
	});

	for (QDomElement& e: config)
	{
		if (e == "initialization")
		{
			for (QDomElement& pmtk: e)
			{
				if (pmtk == "pmtk")
					_boot_pmtk_commands.push_back (pmtk.text().toStdString());
				else
					throw xf::BadDomElement (pmtk);
			}
		}
	}

	_power_cycle_timer = std::make_unique<QTimer> (this);
	_power_cycle_timer->setInterval (kPowerRestartDelay.quantity<Millisecond>());
	_power_cycle_timer->setSingleShot (true);
	QObject::connect (_power_cycle_timer.get(), SIGNAL (timeout()), this, SLOT (power_on()));

	_read_errors.write (0);
	_serviceable.write (false);
	_power_on.write (false);

	power_on();
}


GPS::~GPS()
{
	_power_cycle.reset();
}


void
GPS::data_updated()
{
	// _power_cycle management is done here, since this method is called
	// from main Qt event loop.

	if (_power_cycle_requested)
	{
		_power_cycle_requested = false;
		reset_data_properties();
		_power_cycle.reset();
		_power_cycle_timer->start();
	}

	if (_power_cycle)
		_power_cycle->data_updated();
}


void
GPS::power_on()
{
	++_power_cycle_attempts;
	_power_cycle = std::make_unique<PowerCycle> (*this);
}


void
GPS::request_power_cycle()
{
	_power_cycle_requested = true;
}


void
GPS::reset_data_properties()
{
	_fix_quality.set_nil();
	_fix_mode.set_nil();
	_latitude.set_nil();
	_longitude.set_nil();
	_altitude_amsl.set_nil();
	_geoid_height.set_nil();
	_ground_speed.set_nil();
	_track_true.set_nil();
	_tracked_satellites.set_nil();
	_magnetic_declination.set_nil();
	_hdop.set_nil();
	_vdop.set_nil();
	_pdop.set_nil();
	_lateral_stddev.set_nil();
	_vertical_stddev.set_nil();
	_position_stddev.set_nil();
	_dgps_station_id.set_nil();
	_fix_system_timestamp.set_nil();
	_fix_gps_timestamp.set_nil();
}


void
GPS::update_clock (xf::nmea::GPSDate const& date, xf::nmea::GPSTimeOfDay const& time)
{
	try {
		Time unix_time = xf::nmea::to_unix_time (date, time);
		// Synchronize OS clock only once:
		if (_synchronize_system_clock)
		{
			if (module_manager()->xefis()->system()->set_clock (unix_time))
				log() << "System clock synchronized from GPS." << std::endl;
			_synchronize_system_clock = false;
		}
	}
	catch (xf::nmea::BadDateTime const& e)
	{
		log() << "Could not use date/time information from GPS (invalid data): error message follows:" << std::endl
			  << "  " + std::string (e.what()) << std::endl;
	}
}

