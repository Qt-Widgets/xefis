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
#include <xefis/utility/qdom.h>
#include <xefis/utility/numeric.h>
#include <xefis/utility/range.h>

// Local:
#include "afcs_at.h"


AFCS_AT::AFCS_AT (std::unique_ptr<AFCS_AT_IO> module_io, std::string const& instance):
	Module (std::move (module_io), instance)
{
	_ias_pid.set_integral_limit ({ -5.0_m, +5.0_m });

	_thrust_computer.set_minimum_dt (5_ms);
	_thrust_computer.set_callback (std::bind (&AFCS_AT::compute_thrust, this));
	_thrust_computer.add_depending_smoothers ({
		&_ias_pid_smoother,
	});
	_thrust_computer.observe ({
		&io.input_cmd_speed_mode,
		&io.input_cmd_thrust,
		&io.input_cmd_ias,
		&io.input_measured_ias,
	});
}


void
AFCS_AT::initialize()
{
	_ias_pid.set_pid (*io.setting_ias_pid_settings);
	_ias_pid.set_gain (*io.setting_ias_pid_gain);
}


void
AFCS_AT::process (v2::Cycle const& cycle)
{
	_thrust_computer.process (cycle.update_time());
}


void
AFCS_AT::compute_thrust()
{
	bool disengage = false;
	si::Force computed_thrust = 0.0_N;
	si::Time dt = _thrust_computer.update_dt();

	if (io.input_cmd_speed_mode)
	{
		switch (*io.input_cmd_speed_mode)
		{
			case AFCS_AT_IO::SpeedMode::Thrust:
				if (io.input_cmd_thrust)
					computed_thrust = *io.input_cmd_thrust;
				else
					disengage = true;
				break;

			case AFCS_AT_IO::SpeedMode::Airspeed:
				if (io.input_cmd_ias && io.input_measured_ias)
				{
					// This is more tricky, since we measure IAS, but control thrust.
					// There's no 1:1 correlaction between them.
					// TODO use _ias_pid.set_output_limit (...);
					xf::Range<si::Force> output_extents { *io.setting_output_thrust_minimum, *io.setting_output_thrust_maximum };
					computed_thrust = xf::clamped (_ias_pid_smoother (_ias_pid (*io.input_cmd_ias, *io.input_measured_ias, dt), dt),
												   output_extents);
					// TODO make PID control the change rate of thrust, not the thrust directly. Maybe incorporate
					// something into the PIDControl object itself? Or create another function-like class.
				}
				else
				{
					_ias_pid.reset();
					_ias_pid_smoother.reset();
					disengage = true;
				}
				break;

			case AFCS_AT_IO::SpeedMode::None:
				// Don't change current state:
				break;
		}

		io.output_thrust = computed_thrust;
	}

	if (disengage || !io.disengage_at)
		io.disengage_at = disengage;
}

