// Copyright (C) 2026 CharOfString <root@charofstring.cc>
//
// This program is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation, either version 3 of the License, or (at your option)
// any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
// more details.
//
// You should have received a copy of the GNU General Public License along with
// this program. If not, see <https://www.gnu.org/licenses/>.

#include "LockTranslations.h"

#include <memory>
#include <vector>

#include <QCoreApplication>
#include <QDebug>
#include <QGuiApplication>
#include <QLibraryInfo>
#include <QTranslator>

#include "Constants.h"

namespace gxde::lock {

namespace {

std::vector<std::unique_ptr<QTranslator>>& Installed() {
  static std::vector<std::unique_ptr<QTranslator>> translators;
  return translators;
}

bool Install(const QLocale& locale, const QString& name, const QString& dir) {
  auto translator = std::make_unique<QTranslator>();
  if (!translator->load(locale, name, QStringLiteral("_"), dir)) return false;
  QCoreApplication::installTranslator(translator.get());
  Installed().push_back(std::move(translator));
  return true;
}

}  // namespace

void InstallTranslations(const QLocale& locale) {
  for (const auto& translator : Installed())
    QCoreApplication::removeTranslator(translator.get());
  Installed().clear();

  // Installed last wins, so go from the most generic to the most specific.
  Install(locale, QStringLiteral("gxde-session-ui"),
          QStringLiteral("/usr/share/gxde-session-ui/translations"));
  Install(locale, QStringLiteral("qtbase"),
          QLibraryInfo::path(QLibraryInfo::TranslationsPath));
  const QString dir = qEnvironmentVariable(
      "GXDE_LOCK_TRANSLATIONS", QStringLiteral(TRANSLATIONS_DIR));
  const bool own = Install(locale, QStringLiteral("gxde-lock"), dir);
  if (!own && locale.language() != QLocale::English &&
      locale.language() != QLocale::C) {
    qInfo() << "(Lock) No gxde-lock translation for" << locale.uiLanguages()
            << "in" << dir;
  }

  // Dates and numbers follow the same language. Keep the system locale
  // object itself when it is the one asked for: it honours LC_TIME and
  // friends separately from LC_MESSAGES.
  QLocale::setDefault(locale.name() == QLocale::system().name()
                          ? QLocale::system()
                          : locale);

  if (qobject_cast<QGuiApplication*>(QCoreApplication::instance())) {
    QGuiApplication::setLayoutDirection(own ? locale.textDirection()
                                            : Qt::LeftToRight);
  }
}

}  // namespace gxde::lock
