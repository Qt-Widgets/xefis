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

#ifndef XEFIS__MODULES__SYSTEMS__AFCS_EAC_YD_H__INCLUDED
#define XEFIS__MODULES__SYSTEMS__AFCS_EAC_YD_H__INCLUDED

// Standard:
#include <cstddef>

// Xefis:
#include <xefis/config/all.h>
#include <xefis/core/module.h>
#include <xefis/core/property.h>
#include <xefis/core/property_observer.h>
#include <xefis/utility/pid_control.h>


/**
 * Controls rudder to obtain zero slip-skid value.
 */
class AFCS_EAC_YD: public xf::Module
{
  public:
	// Ctor
	AFCS_EAC_YD (xf::ModuleManager*, QDomElement const& config);

  private:
	// Module API
	void
	data_updated() override;

	/**
	 * Compute rudder.
	 */
	void
	compute();

  private:
	xf::PIDControl<double>	_rudder_pid;
	// Settings:
	double					_rudder_p;
	double					_rudder_i;
	double					_rudder_d;
	double					_rudder_gain;
	double					_limit;
	// Input:
	xf::PropertyBoolean		_input_enabled;
	xf::PropertyFloat		_input_slip_skid_g;
	// Output:
	xf::PropertyFloat		_output_rudder;
	// Other:
	xf::PropertyObserver	_rudder_computer;
};

#endif
