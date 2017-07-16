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

#ifndef XEFIS__MODULES__INSTRUMENTS__GEAR_H__INCLUDED
#define XEFIS__MODULES__INSTRUMENTS__GEAR_H__INCLUDED

// Standard:
#include <cstddef>

// Qt:
#include <QtWidgets/QWidget>
#include <QtXml/QDomElement>

// Xefis:
#include <xefis/config/all.h>
#include <xefis/core/instrument.h>
#include <xefis/core/instrument_aids.h>
#include <xefis/core/v1/property.h>


class Gear:
	public xf::Instrument,
	protected xf::InstrumentAids
{
  public:
	// Ctor
	Gear (xf::ModuleManager*, QDomElement const& config);

	void
	data_updated() override;

  protected:
	void
	resizeEvent (QResizeEvent*) override;

	void
	paintEvent (QPaintEvent*) override;

  private:
	// Properties:
	xf::PropertyBoolean	_setting_down;
	xf::PropertyBoolean	_nose_up;
	xf::PropertyBoolean	_nose_down;
	xf::PropertyBoolean	_left_up;
	xf::PropertyBoolean	_left_down;
	xf::PropertyBoolean	_right_up;
	xf::PropertyBoolean	_right_down;
};

#endif
