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

#ifndef XEFIS__MODULES__HELPERS__MIXER_H__INCLUDED
#define XEFIS__MODULES__HELPERS__MIXER_H__INCLUDED

// Standard:
#include <cstddef>

// Xefis:
#include <xefis/config/all.h>
#include <xefis/core/v2/module.h>
#include <xefis/core/v2/property.h>
#include <xefis/core/v2/setting.h>
#include <xefis/utility/v2/actions.h>


template<class pValue>
	class Mixer: public x2::Module
	{
	  public:
		using Value = pValue;

	  public:
		/*
		 * Settings
		 */

		x2::Setting<double>		setting_input_a_factor	{ this, 1.0 };
		x2::Setting<double>		setting_input_b_factor	{ this, 1.0 };
		x2::Setting<Value>		setting_output_minimum	{ this, x2::BasicSetting::Optional };
		x2::Setting<Value>		setting_output_maximum	{ this, x2::BasicSetting::Optional };

		/*
		 * Input
		 */

		x2::PropertyIn<Value>	input_a_value			{ this, "/input.a" };
		x2::PropertyIn<Value>	input_b_value			{ this, "/input.b" };

		/*
		 * Output
		 */

		x2::PropertyOut<Value>	output_value			{ this, "/value" };

	  public:
		// Ctor
		explicit
		Mixer (std::string const& instance = {});

	  protected:
		// Module API
		void
		initialize() override;

		// Module API
		void
		process (x2::Cycle const&) override;

	  private:
		x2::PropChanged<Value>	_input_a_changed	{ input_a_value };
		x2::PropChanged<Value>	_input_b_changed	{ input_b_value };
	};


/*
 * Implementation
 */


template<class V>
	Mixer<V>::Mixer (std::string const& instance):
		Module (instance)
	{ }


template<class V>
	void
	Mixer<V>::initialize()
	{
		if (setting_output_minimum && setting_output_maximum && *setting_output_minimum > *setting_output_maximum)
			log() << "Settings error: maximum value is less than the minimum value." << std::endl;
	}


template<class V>
	void
	Mixer<V>::process (x2::Cycle const&)
	{
		if (_input_a_changed() || _input_b_changed())
		{
			auto& a = input_a_value;
			auto& b = input_b_value;

			if (a || b)
			{
				Value sum{};

				if (a)
					sum += *setting_input_a_factor * *a;
				if (b)
					sum += *setting_input_b_factor * *b;

				if (setting_output_minimum && sum < *setting_output_minimum)
					sum = *setting_output_minimum;

				if (setting_output_maximum && sum > *setting_output_maximum)
					sum = *setting_output_maximum;

				output_value = sum;
			}
			else
				output_value.set_nil();
		}
	}

#endif
