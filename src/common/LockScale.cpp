/*
 * Copyright (C) CharOfString <root@charofstring.cc>
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include "LockScale.h"

#include <QByteArrayList>
#include <QDebug>
#include <QFile>

#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>

namespace {

double outputScaleRatio(const XRRCrtcInfo* crtcInfo,
                        const XRROutputInfo* outputInfo) {
  const double ratio = static_cast<double>(crtcInfo->width) /
                       static_cast<double>(outputInfo->mm_width) /
                       (1366.0 / 310.0);

  if (ratio > 1 + 2.0 / 3.0) return 2.0;
  if (ratio > 1 + 1.0 / 3.0) return 1.5;
  return 1.0;
}

bool scaleIsSet() {
  return qEnvironmentVariableIsSet("QT_SCALE_FACTOR") ||
         qEnvironmentVariableIsSet("QT_SCREEN_SCALE_FACTORS");
}

}

namespace LockScale {

QString themeConfigPath() {
  return QStringLiteral("/etc/lightdm/deepin/qt-theme.ini");
}

QPair<QByteArray, QByteArray> themeScaleEnvironment(const QString& path) {
  QFile file(path.isEmpty() ? themeConfigPath() : path);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return {};

  QByteArray screenFactors;
  QByteArray factor;
  bool inTheme = false;
  while (!file.atEnd()) {
    const QByteArray line = file.readLine().trimmed();
    if (line.startsWith('[')) {
      inTheme = line == "[Theme]";
      continue;
    }
    const int separator = line.indexOf('=');
    if (!inTheme || separator <= 0) continue;

    const QByteArray key = line.left(separator).trimmed();
    const QByteArray value = line.mid(separator + 1).trimmed();
    if (key == "ScreenScaleFactors") screenFactors = value;
    if (key == "ScaleFactor") factor = value;
  }

  if (screenFactors.contains('=') || screenFactors.contains(';')) {
    return {"QT_SCREEN_SCALE_FACTORS", screenFactors};
  }
  for (const QByteArray& value : {screenFactors, factor}) {
    bool ok = false;
    const double number = value.toDouble(&ok);
    if (ok && number > 0) return {"QT_SCALE_FACTOR", value};
  }
  return {};
}

void applyAutoScreenScaleFactors() {
  if (scaleIsSet()) return;

  Display* display = XOpenDisplay(nullptr);
  if (!display) {
    qputenv("QT_AUTO_SCREEN_SCALE_FACTOR", "1");
    return;
  }

  XRRScreenResources* resources =
    XRRGetScreenResourcesCurrent(display, DefaultRootWindow(display));
  if (!resources) {
    resources = XRRGetScreenResources(display, DefaultRootWindow(display));
    qWarning() << "XRRGetScreenResourcesCurrent failed, using "
                  "XRRGetScreenResources.";
  }

  QByteArrayList factors;
  if (resources) {
    for (int i = 0; i < resources->noutput; ++i) {
      XRROutputInfo* outputInfo =
        XRRGetOutputInfo(display, resources, resources->outputs[i]);
      if (!outputInfo) continue;

      if (outputInfo->crtc != 0 && outputInfo->mm_width > 0) {
        XRRCrtcInfo* crtcInfo =
          XRRGetCrtcInfo(display, resources, outputInfo->crtc);
        if (crtcInfo) {
          const QByteArray outputName(outputInfo->name, outputInfo->nameLen);
          const double ratio = outputScaleRatio(crtcInfo, outputInfo);
          factors.append(outputName + '=' + QByteArray::number(ratio));
          XRRFreeCrtcInfo(crtcInfo);
        }
      }

      XRRFreeOutputInfo(outputInfo);
    }
    XRRFreeScreenResources(resources);
  }
  XCloseDisplay(display);

  if (factors.isEmpty()) {
    qputenv("QT_AUTO_SCREEN_SCALE_FACTOR", "1");
    return;
  }

  const QByteArray screenScaleFactors = factors.join(';');
  qInfo() << "Using per-screen scale factors:" << screenScaleFactors;
  qputenv("QT_SCREEN_SCALE_FACTORS", screenScaleFactors);
}

bool applyThemeScale(const QString& path) {
  if (scaleIsSet()) {
    return true;
  }

  const QPair<QByteArray, QByteArray> themeScale = themeScaleEnvironment(path);
  if (themeScale.first.isEmpty()) {
    return false;
  }
  qInfo() << "Using greeter theme scale:" << themeScale.first << "="
          << themeScale.second;
  qputenv(themeScale.first.constData(), themeScale.second);
  return true;
}

void applyX11ScaleFactors() {
  if (!applyThemeScale()) {
    applyAutoScreenScaleFactors();
  }
}

}
