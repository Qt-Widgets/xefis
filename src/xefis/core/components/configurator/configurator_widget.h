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

#ifndef XEFIS__CORE__COMPONENTS__CONFIGURATOR__CONFIGURATOR_WIDGET_H__INCLUDED
#define XEFIS__CORE__COMPONENTS__CONFIGURATOR__CONFIGURATOR_WIDGET_H__INCLUDED

// Standard:
#include <cstddef>

// Qt:
#include <QtWidgets/QWidget>
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QLabel>

// Xefis:
#include <xefis/config/all.h>
#include <xefis/core/components/property_editor/property_editor.h>
#include <xefis/core/components/data_recorder/data_recorder.h>
#include <xefis/core/window.h>

// Local:
#include "modules_list.h"


namespace xf {

class ConfiguratorWidget: public QWidget
{
	Q_OBJECT

	/**
	 * OwnershipBreakingDecorator widget that ensures its child widget is NOT deleted
	 * when decorator is deleted. Used to break Qt's parent-child relationship
	 * when it comes to pointer ownership (since Qt doesn't have its own
	 * mechanism for this).
	 *
	 * Also - lays out the child widget.
	 */
	class OwnershipBreakingDecorator: public QWidget
	{
	  public:
		// Ctor
		OwnershipBreakingDecorator (QWidget* child, QWidget* parent);

		// Dtor
		~OwnershipBreakingDecorator();

	  private:
		QWidget* _child;
	};

	/**
	 * Configuration widget for module.
	 * Contains generic config widgets, module's configurator widget,
	 * and other stuff.
	 */
	class GeneralModuleWidget: public QWidget
	{
	  public:
		// Ctor
		GeneralModuleWidget (Xefis*, Module*, ConfiguratorWidget*, QWidget* parent);

		/**
		 * Return module.
		 */
		Module*
		module() const noexcept;

		/**
		 * Reload module.
		 */
		void
		reload_module();

	  private:
		Xefis*				_xefis;
		Module*				_module;
		Module::Pointer		_module_ptr;
		ConfiguratorWidget*	_configurator_widget;
	};

  public:
	// Ctor
	ConfiguratorWidget (Xefis*, QWidget* parent);

	Window*
	owning_window() const;

	void
	set_owning_window (Window*);

  private:
	void
	module_selected (Module::Pointer const&);

	void
	none_selected();

	/**
	 * Causes module widget to be reloaded.
	 * The one passed in parameter will be deleted.
	 */
	void
	reload_module_widget (GeneralModuleWidget*);

  private:
	Xefis*				_xefis					= nullptr;
	PropertyEditor*		_property_editor		= nullptr;
	ModulesList*		_modules_list			= nullptr;
	QStackedWidget*		_modules_stack			= nullptr;
	DataRecorder*		_data_recorder			= nullptr;
	QTabWidget*			_tabs					= nullptr;
	Window*				_owning_window			= nullptr;
	QLabel*				_no_module_selected		= nullptr;
	std::map<Module*, Shared<GeneralModuleWidget>>
						_general_module_widgets;
};


inline Module*
ConfiguratorWidget::GeneralModuleWidget::module() const noexcept
{
	return _module;
}


inline Window*
ConfiguratorWidget::owning_window() const
{
	return _owning_window;
}


inline void
ConfiguratorWidget::set_owning_window (Window* window)
{
	_owning_window = window;
}

} // namespace xf

#endif

