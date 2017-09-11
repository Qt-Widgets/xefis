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
#include <math/math.h>

// Local:
#include "imu_cff.h"


IMU_CFF::IMU_CFF (std::unique_ptr<IMU_CFF_IO> module_io, std::string const& instance):
	Module (std::move (module_io), instance)
{
	_centripetal_computer.set_callback (std::bind (&IMU_CFF::compute_centripetal, this));
	_centripetal_computer.add_depending_smoothers ({
		&_smooth_accel_x,
		&_smooth_accel_y,
		&_smooth_accel_z,
	});
	_centripetal_computer.observe ({
		&io.angular_velocity_x,
		&io.angular_velocity_y,
		&io.angular_velocity_z,
		&io.tas_x,
		&io.tas_y,
		&io.tas_z,
	});
}


void
IMU_CFF::process (v2::Cycle const& cycle)
{
	_centripetal_computer.process (cycle.update_time());
}


void
IMU_CFF::compute_centripetal()
{
	/*
	 * Turn radius:
	 *   r = v / (2 * pi * f)
	 * r ← radius
	 * f ← frequency
	 * v ← tas
	 *
	 * Also:
	 *   a = v² / r
	 * Therefore:
	 *   a = v * 2 * pi * f
	 * And:
	 *   a[y] = v[x] * (2 * pi * -f[z])
	 *   a[z] = v[x] * (2 * pi * +f[y])
	 */

	if (io.angular_velocity_x && io.angular_velocity_y && io.angular_velocity_z &&
		io.tas_x && io.tas_y && io.tas_z)
	{
		si::Time dt = _centripetal_computer.update_dt();

		math::Vector<si::Velocity, 3> vec_v {
			*io.tas_x,
			*io.tas_y,
			*io.tas_z,
		};

		math::Vector<si::AngularVelocity, 3> vec_w {
			*io.angular_velocity_x,
			*io.angular_velocity_y,
			*io.angular_velocity_z,
		};

		auto acceleration = math::cross_product (vec_v, vec_w);

		io.centripetal_acceleration_x = _smooth_accel_x (acceleration[0], dt);
		io.centripetal_acceleration_y = _smooth_accel_y (acceleration[1], dt);
		io.centripetal_acceleration_z = _smooth_accel_z (acceleration[2], dt);

		if (io.mass)
		{
			io.centripetal_force_x = *io.mass * *io.centripetal_acceleration_x;
			io.centripetal_force_y = *io.mass * *io.centripetal_acceleration_y;
			io.centripetal_force_z = *io.mass * *io.centripetal_acceleration_z;
		}
		else
		{
			io.centripetal_force_x.set_nil();
			io.centripetal_force_y.set_nil();
			io.centripetal_force_z.set_nil();
		}
	}
	else
	{
		io.centripetal_force_x.set_nil();
		io.centripetal_force_y.set_nil();
		io.centripetal_force_z.set_nil();

		io.centripetal_acceleration_x.set_nil();
		io.centripetal_acceleration_y.set_nil();
		io.centripetal_acceleration_z.set_nil();
	}
}

