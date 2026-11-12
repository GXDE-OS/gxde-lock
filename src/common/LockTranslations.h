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

#ifndef SRC_COMMON_LOCKTRANSLATIONS_H_
#define SRC_COMMON_LOCKTRANSLATIONS_H_

#include <QLocale>

namespace gxde::lock {

// Installs the translators for |locale| on the running application, replacing
// any installed by an earlier call (the classic locker switches to the
// locked user's language). Lookup order, most specific first:
//   1. gxde-lock's own catalog (TRANSLATIONS_DIR, or GXDE_LOCK_TRANSLATIONS),
//   2. Qt's own strings (qtbase),
//   3. the legacy gxde-session-ui catalog, a fallback for the inherited DDE
//      widget strings in languages gxde-lock does not ship.
// Languages are matched against QLocale::uiLanguages(), so LANGUAGE's
// priority list is honoured. Also makes |locale| the default QLocale, and
// right-to-left languages mirror the layout.
void InstallTranslations(const QLocale& locale = QLocale::system());

}  // namespace gxde::lock

#endif  // SRC_COMMON_LOCKTRANSLATIONS_H_
