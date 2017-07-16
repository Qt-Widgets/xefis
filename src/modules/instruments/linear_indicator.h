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

#ifndef XEFIS__MODULES__INSTRUMENTS__LINEAR_INDICATOR_H__INCLUDED
#define XEFIS__MODULES__INSTRUMENTS__LINEAR_INDICATOR_H__INCLUDED

// Standard:
#include <cstddef>

// Qt:
#include <QtWidgets/QWidget>
#include <QtXml/QDomElement>

// Xefis:
#include <xefis/config/all.h>
#include <xefis/core/instrument.h>
#include <xefis/core/v1/property.h>

// Local:
#include "linear_indicator_widget.h"


class LinearIndicator: public xf::Instrument
{
  public:
	// Ctor
	LinearIndicator (xf::ModuleManager*, QDomElement const& config);

	void
	data_updated() override;

  private:
	LinearIndicatorWidget*				_widget = nullptr;
	// Settings:
	bool								_initialize					= true;
	bool								_style_mirrored				= false;
	int									_value_precision			= 0;
	unsigned int						_value_modulo				= 0;
	unsigned int						_value_digits				= 3;
	std::string							_unit;
	xf::PropertyFloat::Type				_value_minimum;
	Optional<xf::PropertyFloat::Type>	_value_minimum_critical;
	Optional<xf::PropertyFloat::Type>	_value_minimum_warning;
	Optional<xf::PropertyFloat::Type>	_value_maximum_warning;
	Optional<xf::PropertyFloat::Type>	_value_maximum_critical;
	xf::PropertyFloat::Type				_value_maximum;
	// Properties:
	xf::GenericProperty					_value;
};

#endif
