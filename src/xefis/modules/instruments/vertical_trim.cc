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
#include <optional>

// Lib:
#include <boost/format.hpp>

// Xefis:
#include <xefis/config/all.h>
#include <xefis/core/v1/window.h>
#include <xefis/utility/numeric.h>

// Local:
#include "vertical_trim.h"


VerticalTrim::VerticalTrim (std::string const& instance):
	Instrument (instance),
	InstrumentAids (0.5f)
{
	_inputs_observer.set_callback ([&]{ update(); });
	_inputs_observer.observe (input_trim_value);
}


void
VerticalTrim::process (v2::Cycle const& cycle)
{
	_inputs_observer.process (cycle.update_dt());
}


void
VerticalTrim::resizeEvent (QResizeEvent*)
{
	auto xw = dynamic_cast<v1::Window*> (window());
	if (xw)
		set_scaling (xw->pen_scale(), xw->font_scale());

	InstrumentAids::update_sizes (size(), window()->size());
}


void
VerticalTrim::paintEvent (QPaintEvent*)
{
	auto painting_token = get_token (this);
	clear_background();

	std::optional<double> trim = input_trim_value.get_optional();

	if (trim)
		xf::clamp (*trim, -1.0, +1.0);

	std::optional<double> ref = input_trim_reference.get_optional();
	std::optional<double> ref_min = input_trim_reference_minimum.get_optional();
	std::optional<double> ref_max = input_trim_reference_maximum.get_optional();

	double h = _font_13_digit_height;
	double v = height() - h;
	bool within_reference = trim && ref_min && ref_max && (*ref_min <= *trim) && (*trim <= *ref_max);

	QFont nu_nd_font = _font_13;
	QFont label_font = _font_13;
	QFont value_font = _font_18;
	QFont reference_font = _font_13;
	QColor cyan = _std_cyan;
	QTransform center_point_transform;
	center_point_transform.translate (0.65 * width(), 0.5 * height());

	// Scale line:
	QPointF nd (-h, 0.5 * (h - height()));
	QPointF nu (-h, 0.5 * (height() - h));
	QPolygonF line = QPolygonF()
		<< nd + QPointF (0.5 * h, 0.0)
		<< nd + QPointF (h, 0.0)
		<< nu + QPointF (h, 0.0)
		<< nu + QPointF (0.5 * h, 0.0);
	painter().setPen (get_pen (Qt::white, 1.0));
	painter().setFont (nu_nd_font);
	painter().setTransform (center_point_transform);
	painter().drawPolyline (line);
	painter().drawLine (QPointF (-0.5 * h, 0.0), QPointF (+0.5 * h, 0.0));
	painter().fast_draw_text (nd - QPointF (0.25 * h, 0.0), Qt::AlignVCenter | Qt::AlignRight, "ND");
	painter().fast_draw_text (nu - QPointF (0.25 * h, 0.0), Qt::AlignVCenter | Qt::AlignRight, "NU");

	// Reference range:
	if (ref_min && ref_max)
	{
		painter().setPen (Qt::NoPen);
		painter().setBrush (Qt::green);
		painter().drawRect (QRectF (QPointF (pen_width (0.5), -*ref_min * 0.5 * v),
									QPointF (pen_width (5.0), -*ref_max * 0.5 * v)));
	}

	// Reference value:
	if (ref)
	{
		painter().setPen (get_pen (_autopilot_color, 2.0));
		painter().add_shadow ([&] {
			painter().drawLine (QPointF (pen_width (0.5), -*ref * 0.5 * v),
								QPointF (pen_width (7.5), -*ref * 0.5 * v));
		});
	}

	// Cyan vertical text:
	painter().setFont (label_font);
	painter().setPen (cyan);
	painter().fast_draw_vertical_text (QPointF (1.5 * h, 0.0), Qt::AlignVCenter | Qt::AlignLeft, *this->label);

	// Pointer:
	if (trim)
	{
		QPolygonF triangle = QPolygonF()
			<< QPointF (-1.0 * h, -0.35 * h)
			<< QPointF (0.0, 0.0)
			<< QPointF (-1.0 * h, +0.35 * h);
		triangle << triangle[0];
		QColor color = within_reference ? Qt::green : Qt::white;
		painter().setPen (get_pen (color, 1.0));
		painter().setBrush (color);
		painter().add_shadow ([&] {
			painter().drawPolygon (triangle.translated (0.0, -*trim * 0.5 * v));
		});
	}

	// Numerical value:
	QString value_str = "   ";
	if (trim)
		value_str = stringify (-*trim);
	double x = 0.25 * h;
	QPointF text_hook = QPointF (-2.0 * h, 0.0);
	Qt::Alignment alignment = Qt::AlignVCenter | Qt::AlignRight;
	painter().setPen (get_pen (within_reference ? Qt::green : Qt::white, 1.0));
	painter().setBrush (Qt::NoBrush);
	painter().setFont (value_font);
	QRectF box = painter().get_text_box (text_hook, alignment, value_str).adjusted (-x, 0.0, x, 0.0);
	painter().fast_draw_text (text_hook, alignment, value_str);
	painter().drawRect (box);

	// Numerical reference:
	if (ref)
	{
		QString ref_str = stringify (-*ref);
		painter().setPen (get_pen (_autopilot_color, 1.0));
		painter().setFont (reference_font);
		painter().fast_draw_text (QPointF (box.center().x(), box.top()), Qt::AlignBottom | Qt::AlignHCenter, ref_str);
	}
}


QString
VerticalTrim::stringify (double value)
{
	QString result = QString::fromStdString ((boost::format ("%+03d") % (std::round (100.0 * value))).str());
	// Remove +/- sign when 00:
	if (result.mid (1, 2) == "00")
		result[0] = ' ';
	return result;
}

