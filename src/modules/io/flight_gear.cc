/* vim:ts=4
 *
 * Copyleft 2012…2013  Michał Gawron
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

// Xefis:
#include <xefis/config/all.h>
#include <xefis/application/application.h>
#include <xefis/application/services.h>
#include <xefis/utility/numeric.h>
#include <xefis/utility/text_painter.h>
#include <xefis/utility/qdom.h>

// Local:
#include "flight_gear.h"


struct FGData
{
	typedef float		FGFloat;
	typedef double		FGDouble;
	typedef uint8_t		FGBool;
	typedef uint32_t	FGInt;

	FGDouble	autopilot_alt_setting_ft;		// apa
	FGDouble	autopilot_cbr_setting_fpm;		// apc
	FGDouble	autopilot_speed_setting_kt;		// ats
	FGDouble	autopilot_heading_setting_deg;	// aph
	FGDouble	flight_director_pitch_deg;		// fdp
	FGDouble	flight_director_roll_deg;		// fdr
	FGDouble	ias_kt;							// ias
	FGDouble	tas_kt;							// tas
	FGDouble	gs_kt;							// gs
	FGDouble	mach;							// ma
	FGDouble	ias_lookahead_kt;				// iasl
	FGDouble	maximum_ias_kt;					// iasma
	FGDouble	minimum_ias_kt;					// iasmi
	FGDouble	altitude_ft;					// al
	FGDouble	altitude_agl_ft;				// alr
	FGDouble	pressure_inhg;					// als
	FGDouble	cbr_fpm;						// cbr
	FGDouble	position_lat_deg;				// lt
	FGDouble	position_lng_deg;				// ln
	FGDouble	position_sea_level_radius_ft;	// slr
	FGDouble	pitch_deg;						// p
	FGDouble	roll_deg;						// r
	FGDouble	mag_heading_deg;				// h
	FGDouble	true_heading_deg;				// th
	FGDouble	fpm_alpha_deg;					// fpa
	FGDouble	fpm_beta_deg;					// fpb
	FGDouble	track_deg;						// tr
	FGBool		navigation_needles_visible;		// nav
	FGBool		navigation_gs_needle_ok;		// ngso
	FGDouble	navigation_gs_needle;			// ngs
	FGBool		navigation_hd_needle_ok;		// nhdo
	FGDouble	navigation_hd_needle;			// nhd
	FGBool		navigation_dme_ok;				// dok
	FGDouble	dme_distance_nm;				// dme
	FGDouble	slip_skid_g;					// ss
	FGDouble	engine_throttle_pct;			// thr
	FGDouble	engine_epr;						// epr
	FGDouble	engine_n1_pct;					// n1
	FGDouble	engine_n2_pct;					// n2
	FGDouble	engine_egt_degc;				// egt
	FGDouble	wind_from_mag_heading_deg;		// wfh
	FGDouble	wind_tas_kt;					// ws
} __attribute__((packed));


FlightGearIO::FlightGearIO (Xefis::ModuleManager* module_manager, QDomElement const& config):
	Input (module_manager)
{
	for (QDomElement& e: config)
	{
		if (e == "input")
		{
			for (QDomElement& e2: e)
			{
				if (e2 == "host")
					_host = e2.text();
				else if (e2 == "port")
					_port = e2.text().toInt();
				else if (e2 == "properties")
				{
					parse_properties (e2, {
						{ "ias", _ias_kt, false },
						{ "ias-lookahead", _ias_lookahead_kt, false },
						{ "ias-minimum", _minimum_ias_kt, false },
						{ "ias-maximum", _maximum_ias_kt, false },
						{ "gs", _gs_kt, false },
						{ "tas", _tas_kt, false },
						{ "mach", _mach, false },
						{ "orientation-pitch", _pitch_deg, false },
						{ "orientation-roll", _roll_deg, false },
						{ "orientation-magnetic-heading", _mag_heading_deg, false },
						{ "orientation-true-heading", _true_heading_deg, false },
						{ "slip-skid", _slip_skid_g, false },
						{ "flight-path-marker-alpha", _fpm_alpha_deg, false },
						{ "flight-path-marker-beta", _fpm_beta_deg, false },
						{ "track", _track_deg, false },
						{ "altitude", _altitude_ft, false },
						{ "altitude-agl", _altitude_agl_ft, false },
						{ "landing-altitude", _landing_altitude_ft, false },
						{ "cbr", _cbr_fpm, false },
						{ "pressure", _pressure_inhg, false },
						{ "autopilot-setting-altitude", _autopilot_alt_setting_ft, false },
						{ "autopilot-setting-ias", _autopilot_speed_setting_kt, false },
						{ "autopilot-setting-heading", _autopilot_heading_setting_deg, false },
						{ "autopilot-setting-cbr", _autopilot_cbr_setting_fpm, false },
						{ "flight-director-pitch", _flight_director_pitch_deg, false },
						{ "flight-director-roll", _flight_director_roll_deg, false },
						{ "navigation-needles-visible", _navigation_needles_visible, false },
						{ "navigation-glide-slope-needle", _navigation_gs_needle, false },
						{ "navigation-heading-needle", _navigation_hd_needle, false },
						{ "dme-distance", _dme_distance_nm, false },
						{ "engine-throttle-pct", _engine_throttle_pct, false },
						{ "engine-epr", _engine_epr, false },
						{ "engine-n1", _engine_n1_pct, false },
						{ "engine-n2", _engine_n2_pct, false },
						{ "engine-egt", _engine_egt_degc, false },
						{ "position-latitude", _position_lat_deg, false },
						{ "position-longitude", _position_lng_deg, false },
						{ "position-sea-level-radius", _position_sea_level_radius_ft, false },
						{ "wind-from-mag-heading", _wind_from_mag_heading_deg, false },
						{ "wind-tas", _wind_tas_kt, false }
					});
				}
			}
		}
	}

	_timeout_timer = new QTimer();
	_timeout_timer->setSingleShot (true);
	_timeout_timer->setInterval (200);
	QObject::connect (_timeout_timer, SIGNAL (timeout()), this, SLOT (invalidate_all()));

	_input = new QUdpSocket();
	_input->bind (QHostAddress (_host), _port, QUdpSocket::ShareAddress);
	QObject::connect (_input, SIGNAL (readyRead()), this, SLOT (read_binary_input()));

	invalidate_all();
}


FlightGearIO::~FlightGearIO()
{
	delete _input;
	delete _timeout_timer;
}


void
FlightGearIO::read_binary_input()
{
	while (_input->hasPendingDatagrams())
	{
		int datagram_size = _input->pendingDatagramSize();
		if (_datagram.size() < datagram_size)
			_datagram.resize (datagram_size);

		_input->readDatagram (_datagram.data(), datagram_size, nullptr, nullptr);
		FGData* fg_data = reinterpret_cast<FGData*> (_datagram.data());

#define ASSIGN(x) \
		if (!_##x.is_singular()) \
			_##x.write (fg_data->x);

		ASSIGN (autopilot_alt_setting_ft);
		ASSIGN (autopilot_cbr_setting_fpm);
		ASSIGN (autopilot_speed_setting_kt);
		ASSIGN (autopilot_heading_setting_deg);
		ASSIGN (flight_director_pitch_deg);
		ASSIGN (flight_director_roll_deg);
		ASSIGN (ias_kt);
		ASSIGN (tas_kt);
		ASSIGN (gs_kt);
		ASSIGN (mach);
		ASSIGN (ias_lookahead_kt);
		ASSIGN (maximum_ias_kt);
		ASSIGN (minimum_ias_kt);
		ASSIGN (altitude_ft);
		ASSIGN (altitude_agl_ft);
		ASSIGN (pressure_inhg);
		ASSIGN (cbr_fpm);
		ASSIGN (position_lat_deg);
		ASSIGN (position_lng_deg);
		ASSIGN (position_sea_level_radius_ft);
		ASSIGN (pitch_deg);
		ASSIGN (roll_deg);
		ASSIGN (mag_heading_deg);
		ASSIGN (true_heading_deg);
		ASSIGN (fpm_alpha_deg);
		ASSIGN (fpm_beta_deg);
		ASSIGN (track_deg);
		ASSIGN (navigation_needles_visible);
		ASSIGN (navigation_gs_needle);
		ASSIGN (navigation_hd_needle);
		ASSIGN (dme_distance_nm);
		ASSIGN (slip_skid_g);
		ASSIGN (engine_throttle_pct);
		ASSIGN (engine_epr);
		ASSIGN (engine_n1_pct);
		ASSIGN (engine_n2_pct);
		ASSIGN (engine_egt_degc);
		ASSIGN (wind_from_mag_heading_deg);
		ASSIGN (wind_tas_kt);

#undef ASSIGN

		if (!fg_data->navigation_gs_needle_ok && !_navigation_gs_needle.is_singular())
			_navigation_gs_needle.set_nil();
		if (!fg_data->navigation_hd_needle_ok && !_navigation_hd_needle.is_singular())
			_navigation_hd_needle.set_nil();
		if (!fg_data->navigation_dme_ok && !_dme_distance_nm.is_singular())
			_dme_distance_nm.set_nil();
	}

	if (_maximum_ias_kt.valid() && *_maximum_ias_kt < 1.f)
		_maximum_ias_kt.set_nil();
	if (_minimum_ias_kt.valid() && *_minimum_ias_kt < 1.f)
		_minimum_ias_kt.set_nil();
	if (_altitude_agl_ft.valid() && *_altitude_agl_ft > 2500.f)
		_altitude_agl_ft.set_nil();
	// Convert EGT from °F to °C:
	if (_engine_egt_degc.valid())
		_engine_egt_degc.write (5.f / 9.f * (*_engine_egt_degc - 32.f));

	data_updated();

	_timeout_timer->start();
}


void
FlightGearIO::invalidate_all()
{
	Xefis::BaseProperty* properties[] = {
		&_ias_kt,
		&_ias_lookahead_kt,
		&_minimum_ias_kt,
		&_maximum_ias_kt,
		&_gs_kt,
		&_tas_kt,
		&_mach,
		&_pitch_deg,
		&_roll_deg,
		&_mag_heading_deg,
		&_true_heading_deg,
		&_slip_skid_g,
		&_fpm_alpha_deg,
		&_fpm_beta_deg,
		&_track_deg,
		&_altitude_ft,
		&_altitude_agl_ft,
		&_landing_altitude_ft,
		&_cbr_fpm,
		&_pressure_inhg,
		&_autopilot_alt_setting_ft,
		&_autopilot_speed_setting_kt,
		&_autopilot_heading_setting_deg,
		&_autopilot_cbr_setting_fpm,
		&_flight_director_pitch_deg,
		&_flight_director_roll_deg,
		&_navigation_needles_visible,
		&_navigation_gs_needle,
		&_navigation_hd_needle,
		&_dme_distance_nm,
		&_engine_throttle_pct,
		&_engine_epr,
		&_engine_n1_pct,
		&_engine_n2_pct,
		&_engine_egt_degc,
		&_position_lat_deg,
		&_position_lng_deg,
		&_position_sea_level_radius_ft,
		&_wind_from_mag_heading_deg,
		&_wind_tas_kt
	};

	for (auto property: properties)
	{
		if (property->valid())
			property->set_nil();
	}

	data_updated();
}
