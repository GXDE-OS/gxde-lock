// Copyright (C) 2026 CharOfString <root@charofstring.cc>
//
// This program is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation, version 3.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
// more details.
//
// You should have received a copy of the GNU General Public License along with
// this program. If not, see <https://www.gnu.org/licenses/>.

import QtQuick
import QtQuick.Window

Window {
  id: root

  property var backend: null
  property bool primaryScreen: true
  property bool virtualKeyboardAvailable: false
  property bool testMode: false
  property bool shown: false
  // X11 production mode with override redirect, bypassing the WM so that nothing
  // draws beyond it, so user's screen shall be "securely" locked.
  property bool x11Overlay: false

  width: testMode ? 1100 : 800
  height: testMode ? 720 : 600
  x: testMode ? 80 : 0
  y: testMode ? 80 : 0
  
  // Anim: From transparent to opaque
  color: lockScreen.unlocking ? "transparent" : "#000000"
  flags: testMode ? Qt.Window
    : (x11Overlay
       ? (Qt.FramelessWindowHint | Qt.X11BypassWindowManagerHint)
       : (Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint))
  visibility: testMode ? Window.Windowed
    : (!shown ? Window.Hidden
       : (x11Overlay ? Window.Windowed : Window.FullScreen))
  title: qsTr("GXDE Lock Screen")

  onClosing: function(close) {
    if (root.testMode) {
      close.accepted = true
      Qt.quit()
      return
    }
    // The production locker owns the session until authentication succeeds.
    // A window close request must never be able to dismiss it.
    close.accepted = false
  }

  LockScreen {
    id: lockScreen

    anchors.fill: parent
    backend: root.backend
    primaryScreen: root.primaryScreen
    virtualKeyboardAvailable: root.virtualKeyboardAvailable
    testMode: root.testMode
    windowVisible: root.visible
    onRequestQuit: Qt.quit()
    onUnlockFadeFinished: {
      if (root.testMode) {
        Qt.quit()
      } else if (root.backend) {
        root.backend.NotifyUnlockFadeFinished()
      }
    }
  }

  function handleAuthenticated() {
    lockScreen.handleAuthenticated()
  }

  function handleAuthenticationFailed(message) {
    lockScreen.handleAuthenticationFailed(message)
  }

  function handlePowerActionStarted(action) {
    lockScreen.handlePowerActionStarted(action)
  }

  function handleShowUserList() {
    lockScreen.handleShowUserList()
  }
}
