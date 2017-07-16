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

#ifndef XEFIS__CORE__INSTRUMENT_H__INCLUDED
#define XEFIS__CORE__INSTRUMENT_H__INCLUDED

// Standard:
#include <cstddef>

// Qt:
#include <QtWidgets/QWidget>
#include <QtXml/QDomElement>

// Xefis:
#include <xefis/config/all.h>
#include <xefis/core/v1/module.h>
#include <xefis/core/services.h>


namespace v1 {
using namespace xf;

class ModuleManager;

class Instrument:
	public Module,
	public QWidget
{
  public:
	// Ctor
	explicit
	Instrument (ModuleManager*, QDomElement const& config);
};


inline
Instrument::Instrument (ModuleManager* module_manager, QDomElement const& config):
	Module (module_manager, config),
	QWidget (nullptr)
{
	setFont (Services::instrument_font());
	setCursor (QCursor (Qt::CrossCursor));
}

} // namespace v1

#endif

