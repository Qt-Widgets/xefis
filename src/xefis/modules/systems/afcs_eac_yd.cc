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

// Local:
#include "afcs_eac_yd.h"


AFCS_EAC_YD::AFCS_EAC_YD (std::string const& instance):
	Module (instance)
{
	_rudder_pid.set_integral_limit ({ -0.1_Ns, +0.1_Ns });
	_rudder_pid.set_output_limit ({ -*setting_deflection_limit, *setting_deflection_limit });

	_rudder_computer.set_callback (std::bind (&AFCS_EAC_YD::compute, this));
	_rudder_computer.observe ({
		&input_enabled,
		&input_slip_skid,
	});
}


void
AFCS_EAC_YD::initialize()
{
	_rudder_pid.set_pid (*setting_rudder_pid_settings);
	_rudder_pid.set_gain (*setting_rudder_pid_gain);
}


void
AFCS_EAC_YD::process (x2::Cycle const& cycle)
{
	_rudder_computer.process (cycle.update_time());
}


void
AFCS_EAC_YD::compute()
{
	si::Time dt = _rudder_computer.update_dt();

	if (input_enabled.value_or (false))
	{
		if (input_slip_skid)
			output_rudder_deflection = _rudder_pid (0.0_N, *input_slip_skid, dt);
		else
			output_rudder_deflection.set_nil();
	}
	else
		output_rudder_deflection = 0.0_deg;
}

