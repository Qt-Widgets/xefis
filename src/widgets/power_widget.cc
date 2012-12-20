/* vim:ts=4
 *
 * Copyleft 2008…2012  Michał Gawron
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
#include <vector>

// Qt:
#include <QtGui/QPainter>

// Xefis:
#include <xefis/config/all.h>
#include <xefis/utility/numeric.h>

// Local:
#include "power_widget.h"


PowerWidget::PowerWidget (QWidget* parent):
	InstrumentWidget (parent, 1.f, 4.5f)
{ }


void
PowerWidget::paintEvent (QPaintEvent* paint_event)
{
	float const w = width();
	float const h = height();

	// Draw on buffer:
	QPixmap buffer (w, h);
	QPainter painter (&buffer);
	TextPainter text_painter (painter);
	painter.setRenderHint (QPainter::Antialiasing, true);
	painter.setRenderHint (QPainter::TextAntialiasing, true);
	painter.setRenderHint (QPainter::SmoothPixmapTransform, true);
	painter.setRenderHint (QPainter::NonCosmeticDefaultPen, true);

	// Clear with black background:
	painter.setPen (Qt::NoPen);
	painter.setBrush (QBrush (QColor (0, 0, 0), Qt::SolidPattern));
	painter.drawRect (rect());

	painter.translate (w / 2.25f, h / 2.f);

	float q = 0.06f * wh();
	float r = 6.5f * q;

	paint_text (painter, text_painter, q, r);
	paint_indicator (painter, text_painter, q, r);

	// Copy buffer to screen:
	QPainter (this).drawPixmap (paint_event->rect().topLeft(), buffer, paint_event->rect());
}


void
PowerWidget::paint_text (QPainter& painter, TextPainter& text_painter, float q, float)
{
	QColor white (255, 255, 255);
	QColor yellow (255, 220, 0);
	QColor red (255, 0, 0);

	QString text = QString ("%1").arg (_value, 0, 'f', 1);

	QFont font (_font_20_bold);
	QFontMetricsF metrics (font);

	QPen pen = get_pen (white, 1.4f);
	pen.setCapStyle (Qt::RoundCap);

	float margin = 0.4f * q;

	QRectF text_rect (0.5f * pen.width(), -0.6f * q, metrics.width ("000.0"), 0.9f * metrics.height());
	text_rect.translate (margin, -text_rect.height());
	QRectF rect = text_rect.adjusted (-margin, 0, margin, 0);

	painter.save();
	painter.setFont (font);
	if (_value_visible)
	{
		float sgn = _range.min() <= _range.max() ? +1.f : -1.f;
		float value = sgn * _value;
		float warning = sgn * _warning_value;
		float critical = sgn * _critical_value;

		if (_warning_visible && value >= warning && value < critical)
			pen.setColor (yellow);
		else if (_critical_visible && value >= critical)
			pen.setColor (red);

		painter.setPen (pen);
		painter.drawRect (rect);
		text_painter.drawText (text_rect, Qt::AlignRight | Qt::AlignVCenter, text);
	}
	else
	{
		painter.setPen (pen);
		painter.drawRect (rect);
	}

	painter.restore();
}


void
PowerWidget::paint_indicator (QPainter& painter, TextPainter&, float, float r)
{
	QColor gray (0x7a, 0x7a, 0x7a);
	QColor yellow (255, 220, 0);
	QColor red (255, 0, 0);

	QPen pen = get_pen (QColor (255, 255, 255), 1.4f);
	pen.setCapStyle (Qt::RoundCap);

	QPen pointer_pen = get_pen (QColor (255, 255, 255), 1.6f);
	pointer_pen.setCapStyle (Qt::RoundCap);

	QPen warning_pen = get_pen (yellow, 1.4f);
	warning_pen.setCapStyle (Qt::RoundCap);

	QPen critical_pen = get_pen (red, 1.4f);
	critical_pen.setCapStyle (Qt::RoundCap);

	QPen green_pen = get_pen (QColor (0x00, 0xff, 0x00), 1.4f);
	green_pen.setCapStyle (Qt::RoundCap);

	QPen gray_pen = get_pen (QColor (0xb0, 0xb0, 0xb0), 1.4f);
	gray_pen.setCapStyle (Qt::RoundCap);

	QBrush brush (gray, Qt::SolidPattern);
	QRectF rect (-r, -r, 2.f * r, 2.f * r);

	float value_span_angle = 210.f;
	float value = bound (_value, _range);
	float warning = bound (_warning_value, _range);
	float critical = bound (_critical_value, _range);
	float normal = bound (_normal_value, _range);

	if (!_warning_visible)
		warning = _range.max();
	if (!_critical_visible)
		critical = _range.max();

	float value_angle = value_span_angle * (value - _range.min()) / _range.extent();
	float warning_angle = value_span_angle * (warning - _range.min()) / _range.extent();
	float critical_angle = value_span_angle * (critical - _range.min()) / _range.extent();
	float normal_angle = value_span_angle * (normal - _range.min()) / _range.extent();

	painter.save();
	if (_value_visible)
	{
		painter.save();
		painter.setPen (Qt::NoPen);
		painter.setBrush (brush);
		painter.drawPie (rect, -16.f * 0.f, -16.f * value_angle);
		painter.setPen (gray_pen);
		painter.drawLine (QPointF (0.f, 0.f), QPointF (r, 0.f));
		painter.rotate (value_angle);
		painter.setPen (pointer_pen);
		painter.drawLine (QPointF (0.f, 0.f), QPointF (0.99f * r, 0.f));
		painter.restore();
	}

	// Warning/critical bugs:

	float gap_degs = 4;
	struct PointInfo { float angle; QPen pen; float tick_len; };
	std::vector<PointInfo> points;

	points.push_back ({ 0.f, pen, 0.f });
	if (_warning_visible)
		points.push_back ({ warning_angle, warning_pen, 0.1f * r });
	if (_critical_visible)
		points.push_back ({ critical_angle, critical_pen, 0.2f * r });
	points.push_back ({ value_span_angle, critical_pen, 0.f });

	for (auto i = 0u; i < points.size() - 1; ++i)
	{
		PointInfo curr = points[i];
		PointInfo next = points[i + 1];
		float is_last = i == points.size() - 2;

		painter.save();
		painter.setPen (curr.pen);
		painter.drawArc (rect,
						 -16.f * curr.angle,
						 -16.f * (next.angle - curr.angle - (is_last ? 0 : gap_degs)));
		painter.rotate (curr.angle);
		painter.drawLine (QPointF (r, 0.f), QPointF (r + curr.tick_len, 0.f));
		painter.restore();
	}

	// Normal value bug:
	if (_normal_visible)
	{
		painter.setPen (green_pen);
		painter.rotate (normal_angle);
		painter.drawLine (QPointF (r + pen_width (1.4f), 0.f), QPointF (1.2f * r, -0.12f * r));
		painter.drawLine (QPointF (r + pen_width (1.4f), 0.f), QPointF (1.2f * r, +0.12f * r));
	}

	painter.restore();
}
