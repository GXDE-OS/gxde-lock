#include "LockCursor.h"

#include "LockAppearance.h"

#include <QByteArrayList>

namespace LockCursor {

void applyConfiguredTheme() {
  const QString theme = LockAppearance::cursorTheme();
  if (!theme.isEmpty()) qputenv("XCURSOR_THEME", theme.toUtf8());
}

void enforceWaylandCursorConfig() {
  const QByteArray variable = "QT_WAYLAND_DISABLED_INTERFACES";
  QByteArrayList disabledInterfaces = qgetenv(variable.constData()).split(',');
  for (QByteArray& interface : disabledInterfaces) {
    interface = interface.trimmed();
  }
  disabledInterfaces.removeAll(QByteArray());

  const QByteArray cursorShapeInterface = "wp_cursor_shape_manager_v1";
  if (!disabledInterfaces.contains(cursorShapeInterface)) {
    disabledInterfaces.append(cursorShapeInterface);
  }
  qputenv(variable.constData(), disabledInterfaces.join(','));
}

}  // namespace LockCursor
