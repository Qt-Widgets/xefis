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

#ifndef XEFIS__MODULES__INSTRUMENTS__DEBUG_FORCES_H__INCLUDED
#define XEFIS__MODULES__INSTRUMENTS__DEBUG_FORCES_H__INCLUDED

// Standard:
#include <cstddef>

// Xefis:
#include <xefis/config/all.h>
#include <xefis/core/v2/instrument.h>
#include <xefis/core/instrument_aids.h>
#include <xefis/core/v2/property.h>


class DebugForcesIO: public v2::ModuleIO
{
  public:
	/*
	 * Input
	 */

	v2::PropertyIn<si::Angle>			orientation_pitch				{ this, "/orientation/pitch" };
	v2::PropertyIn<si::Angle>			orientation_roll				{ this, "/orientation/roll" };
	v2::PropertyIn<si::Angle>			orientation_magnetic_heading	{ this, "/orientation/magnetic-heading" };
	v2::PropertyIn<si::Acceleration>	measured_accel_x				{ this, "/acceleration/x" };
	v2::PropertyIn<si::Acceleration>	measured_accel_y				{ this, "/acceleration/y" };
	v2::PropertyIn<si::Acceleration>	measured_accel_z				{ this, "/acceleration/z" };
	v2::PropertyIn<si::Acceleration>	centrifugal_accel_x				{ this, "/centrifugal-acceleration/x" };
	v2::PropertyIn<si::Acceleration>	centrifugal_accel_y				{ this, "/centrifugal-acceleration/y" };
	v2::PropertyIn<si::Acceleration>	centrifugal_accel_z				{ this, "/centrifugal-acceleration/z" };
};


class DebugForces:
	public v2::Instrument<DebugForcesIO>,
	protected xf::InstrumentAids
{
  public:
	// Ctor
	explicit
	DebugForces (std::unique_ptr<DebugForcesIO>, std::string const& instance = {});

	// Module API
	void
	process (v2::Cycle const&) override;

  protected:
	// QWidget API
	void
	paintEvent (QPaintEvent*) override;
};

#endif
