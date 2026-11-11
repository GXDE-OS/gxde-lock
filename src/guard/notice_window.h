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

#ifndef SRC_GUARD_NOTICE_WINDOW_H_
#define SRC_GUARD_NOTICE_WINDOW_H_

#include <QRasterWindow>
#include <QString>

namespace gxde::lock {

// One output's worth of the crash notice: a dark full-screen surface with the
// unlock instructions. Raster painting only, so it works where the regular
// lock screens' GPU or QML stack is what keeps crashing.
class NoticeWindow : public QRasterWindow {
  Q_OBJECT

 public:
  explicit NoticeWindow(QString text);

 protected:
  void paintEvent(QPaintEvent* event) override;

 private:
  const QString text_;
};

}  // namespace gxde::lock

#endif  // SRC_GUARD_NOTICE_WINDOW_H_
