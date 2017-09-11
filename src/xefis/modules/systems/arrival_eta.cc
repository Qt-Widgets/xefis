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

// Xefis:
#include <xefis/config/all.h>
#include <xefis/support/navigation/earth.h>
#include <xefis/utility/numeric.h>

// Local:
#include "arrival_eta.h"


ArrivalETA::ArrivalETA (std::unique_ptr<ArrivalETA_IO> module_io, std::string const& instance):
	Module (std::move (module_io), instance)
{
	_eta_computer.set_minimum_dt (1_s);
	_eta_computer.set_callback (std::bind (&ArrivalETA::compute, this));
	_eta_computer.add_depending_smoothers ({
		&_smoother,
	});
	_eta_computer.observe ({
		&io.station_latitude,
		&io.station_longitude,
		&io.aircraft_latitude,
		&io.aircraft_longitude,
	});
}


void
ArrivalETA::process (v2::Cycle const& cycle)
{
	_eta_computer.process (cycle.update_time());
}


void
ArrivalETA::compute()
{
	struct SetNil
	{
		SetNil (bool reset_prev_distance):
			reset_prev_distance (reset_prev_distance)
		{ }

		bool reset_prev_distance = false;
	};

	si::Time dt = _eta_computer.update_dt();
	si::Length distance;

	try {
		if (!io.station_latitude || !io.station_longitude ||
			!io.aircraft_latitude || !io.aircraft_longitude ||
			!io.track_lateral_true)
		{
			throw SetNil (true);
		}

		si::LonLat station (*io.station_longitude, *io.station_latitude);
		si::LonLat aircraft (*io.aircraft_longitude, *io.aircraft_latitude);
		distance = xf::haversine_earth (station, aircraft);

		si::Angle station_bearing = xf::floored_mod<si::Angle> (xf::initial_bearing (aircraft, station), 0_deg, 360_deg);
		si::Angle angle_diff = xf::floored_mod<si::Angle> (station_bearing - *io.track_lateral_true, -180_deg, +180_deg);

		if (abs (angle_diff) > 30_deg)
			throw SetNil (true);

		if (_prev_distance)
		{
			si::Length distance_diff = distance - *_prev_distance;

			if (distance_diff >= 0_nmi)
			{
				_prev_distance = distance;
				throw SetNil (false);
			}

			io.eta = _smoother (dt * (-distance / distance_diff), dt);
		}
		else
			io.eta.set_nil();

		_prev_distance = distance;
	}
	catch (SetNil const& set_nil)
	{
		if (set_nil.reset_prev_distance)
			_prev_distance.reset();

		io.eta.set_nil();
	}
}

