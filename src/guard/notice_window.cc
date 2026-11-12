/*
 * Copyright (C) 2026 CharOfString <root@charofstring.cc>
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include "notice_window.h"

#include <utility>

#include <QFont>
#include <QGuiApplication>
#include <QPainter>
#include <QRect>

namespace gxde::lock {

NoticeWindow::NoticeWindow(QString text) : text_(std::move(text)) {
  setTitle(QStringLiteral("GXDE Lock Screen"));
}

void NoticeWindow::paintEvent(QPaintEvent* event) {
  Q_UNUSED(event)
  const QRect area(0, 0, width(), height());
  QPainter painter(this);
  painter.setLayoutDirection(QGuiApplication::layoutDirection());
  painter.fillRect(area, QColor(0x14, 0x16, 0x1c));

  QFont font = painter.font();
  font.setPixelSize(qBound(16, height() / 36, 40));
  painter.setFont(font);
  painter.setPen(QColor(0xee, 0xee, 0xee));

  const int margin = width() / 10;
  painter.drawText(area.adjusted(margin, margin, -margin, -margin),
                   Qt::AlignCenter | Qt::TextWordWrap, text_);
}

}  // namespace gxde::lock
