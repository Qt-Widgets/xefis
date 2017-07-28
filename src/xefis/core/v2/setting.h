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

#ifndef XEFIS__CORE__V2__SETTING_H__INCLUDED
#define XEFIS__CORE__V2__SETTING_H__INCLUDED

// Standard:
#include <cstddef>
#include <cstdint>

// Xefis:
#include <xefis/config/all.h>
#include <xefis/core/v2/module.h>
#include <xefis/core/property_path.h>
#include <xefis/utility/time.h>
#include <xefis/utility/time_helper.h>
#include <xefis/utility/blob.h>


namespace v2 {

class ModuleIO;


/**
 * Common base class for all Setting<*> types.
 */
class BasicSetting
{
  public:
	/**
	 * Tag for creating a setting that doesn't not need to be set by user explicitly, but doesn't necessarily
	 * have any default value.
	 */
	enum OptionalTag { Optional }; // XXX

  public:
	/**
	 * Store pointer to owning module.
	 */
	explicit
	BasicSetting (ModuleIO* owner);

	/**
	 * Return owning module.
	 */
	ModuleIO*
	io() const noexcept;

	/**
	 * Return true if setting has a value.
	 */
	virtual
	operator bool() const noexcept = 0;

  private:
	ModuleIO* _owner;
};


/**
 * Wrapper for setting variables.
 * Allows run-timer checking if required settings have been configured.
 */
template<class pValue>
	class Setting: public BasicSetting
	{
	  public:
		typedef pValue Value;

		class Uninitialized: public xf::Exception
		{
			using Exception::Exception;
		};

	  public:
		/**
		 * Create a setting object that requires explicit setting of a value.
		 */
		explicit
		Setting (ModuleIO* owner);

		/**
		 * Creates a setting object that has an initial value.
		 */
		explicit
		Setting (ModuleIO* owner, Value&& initial_value);

		/**
		 * Creates a setting that doesn't have and doesn't require any value.
		 */
		explicit
		Setting (ModuleIO* owner, OptionalTag);

		/**
		 * Copy-assignment operator.
		 */
		Setting const&
		operator= (Value&& new_value);

		/**
		 * Return true if setting is required to have a value.
		 */
		bool
		required() const noexcept;

		/**
		 * Read the setting value.
		 */
		Value&
		operator*();

		/**
		 * Read the setting value.
		 */
		Value const&
		operator*() const;

		/**
		 * Read the setting value.
		 */
		Value const*
		operator->() const;

		// BasicSetting API
		operator bool() const noexcept override;

	  private:
		std::optional<Value>	_value;
		bool					_required;
	};


inline BasicSetting::BasicSetting (ModuleIO* owner):
	_owner (owner)
{ }


inline ModuleIO*
BasicSetting::io() const noexcept
{
	return _owner;
}


template<class V>
	inline
	Setting<V>::Setting (ModuleIO* owner):
		BasicSetting (owner)
	{ }


template<class V>
	inline
	Setting<V>::Setting (ModuleIO* owner, Value&& initial_value):
		BasicSetting (owner),
		_value (std::forward<Value> (initial_value)),
		_required (true)
	{ }


template<class V>
	inline
	Setting<V>::Setting (ModuleIO* owner, OptionalTag):
		BasicSetting (owner),
		_required (false)
	{ }


template<class V>
	inline Setting<V> const&
	Setting<V>::operator= (Value&& value)
	{
		_value = std::forward<Value> (value);
		return *this;
	}


template<class V>
	inline bool
	Setting<V>::required() const noexcept
	{
		return _required;
	}


template<class V>
	inline auto
	Setting<V>::operator*() -> Value&
	{
		if (!_value)
			throw Uninitialized ("reading uninitialized setting");

		return *_value;
	}


template<class V>
	inline auto
	Setting<V>::operator*() const -> Value const&
	{
		if (!_value)
			throw Uninitialized ("reading uninitialized setting");

		return *_value;
	}


template<class V>
	inline auto
	Setting<V>::operator->() const -> Value const*
	{
		if (!_value)
			throw Uninitialized ("reading uninitialized setting");

		return &_value.value();
	}


template<class V>
	inline
	Setting<V>::operator bool() const noexcept
	{
		return !!_value;
	}

} // namespace v2

#endif

