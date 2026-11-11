#ifndef SRC_COMMON_LOCKCURSOR_H_
#define SRC_COMMON_LOCKCURSOR_H_

#include <QtGlobal>

// Cursor setup shared by gxde-lock-classic and gxde-lock-neo.
namespace LockCursor {

// Exports the cursor theme chosen through gxde-lock's SetCursor (state.conf) as
// XCURSOR_THEME. Call before the QGuiApplication is created.
void applyConfiguredTheme();

// Keeps Qt from handing cursors to a Wayland compositor through
// wp_cursor_shape_manager_v1, so the configured theme is drawn by the
// client. Call before the QGuiApplication is created.
void enforceWaylandCursorConfig();

}  // namespace LockCursor

#endif  // SRC_COMMON_LOCKCURSOR_H_
