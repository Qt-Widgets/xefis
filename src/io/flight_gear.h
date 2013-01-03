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

#ifndef XEFIS__INPUT__FLIGHT_GEAR_H__INCLUDED
#define XEFIS__INPUT__FLIGHT_GEAR_H__INCLUDED

// Standard:
#include <cstddef>
#include <map>

// Qt:
#include <QtCore/QTimer>
#include <QtCore/QObject>
#include <QtNetwork/QUdpSocket>
#include <QtXml/QDomElement>

// Xefis:
#include <xefis/config/all.h>
#include <xefis/core/property.h>
#include <xefis/core/input.h>


class FlightGearIO:
	public QObject,
	public Xefis::Input
{
	Q_OBJECT

	typedef std::map<QString, Xefis::PropertyFloat*> FloatVars;

  public:
	// Ctor
	FlightGearIO (QDomElement const& config);

	// Dtor
	~FlightGearIO();

  private:
	void
	set_path (QString const& path);

  private slots:
	/**
	 * Read and apply FlightGear datagrams from UDP socket.
	 */
	void
	read_input();

	/**
	 * Handle (=assign) float-type variable coming from FlightGear.
	 */
	bool
	handle_float_variable (QString const& variable, QString const& value);

	/**
	 * Set all input properties as invalid.
	 */
	void
	invalidate_all();

  private:
	QTimer*					_timeout_timer = nullptr;
	QUdpSocket*				_input = nullptr;
	std::string				_property_path;
	FloatVars				_float_vars;
	double					_prev_position_lat_deg = 0.f;
	bool					_prev_position_lat_valid = false;
	double					_prev_position_lng_deg = 0.f;
	bool					_prev_position_lng_valid = false;

	Xefis::PropertyFloat	_ias_kt;
	Xefis::PropertyFloat	_ias_lookahead_kt;
	Xefis::PropertyFloat	_minimum_ias_kt;
	Xefis::PropertyFloat	_maximum_ias_kt;
	Xefis::PropertyFloat	_gs_kt;
	Xefis::PropertyFloat	_tas_kt;
	Xefis::PropertyFloat	_mach;
	Xefis::PropertyFloat	_pitch_deg;
	Xefis::PropertyFloat	_roll_deg;
	Xefis::PropertyFloat	_heading_deg;
	Xefis::PropertyFloat	_slip_skid_g;
	Xefis::PropertyFloat	_fpm_alpha_deg;
	Xefis::PropertyFloat	_fpm_beta_deg;
	Xefis::PropertyFloat	_track_deg;
	Xefis::PropertyFloat	_altitude_ft;
	Xefis::PropertyFloat	_altitude_agl_ft;
	Xefis::PropertyFloat	_landing_altitude_ft;
	Xefis::PropertyFloat	_cbr_fpm;
	Xefis::PropertyFloat	_pressure_inhg;
	Xefis::PropertyFloat	_autopilot_alt_setting_ft;
	Xefis::PropertyFloat	_autopilot_speed_setting_kt;
	Xefis::PropertyFloat	_autopilot_heading_setting_deg;
	Xefis::PropertyFloat	_autopilot_cbr_setting_fpm;
	Xefis::PropertyFloat	_flight_director_pitch_deg;
	Xefis::PropertyFloat	_flight_director_roll_deg;
	Xefis::PropertyBoolean	_navigation_needles_visible;
	Xefis::PropertyFloat	_navigation_gs_needle;
	Xefis::PropertyFloat	_navigation_hd_needle;
	Xefis::PropertyFloat	_dme_distance_nm;
	Xefis::PropertyFloat	_engine_throttle_pct;
	Xefis::PropertyFloat	_engine_epr;
	Xefis::PropertyFloat	_engine_n1_pct;
	Xefis::PropertyFloat	_engine_n2_pct;
	Xefis::PropertyFloat	_engine_egt_degc;
	Xefis::PropertyFloat	_position_lat_deg;
	Xefis::PropertyFloat	_position_lng_deg;
	Xefis::PropertyFloat	_position_sea_level_radius_ft;
};

#endif