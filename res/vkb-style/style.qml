// Copyright (C) CharOfString <root@charofstring.cc>
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
import QtQuick.VirtualKeyboard
import QtQuick.VirtualKeyboard.Styles

KeyboardStyle {
  id: currentStyle

  readonly property string resourcePrefix:
    "qrc:/QtQuick/VirtualKeyboard/content/styles/gxde/"
  readonly property string fontFamily: "Source Han Sans"
  readonly property real keyRadius: 6
  readonly property real keyMargin: Math.round(8 * scaleHint)
  readonly property real letterPixelSize: Math.round(60 * scaleHint)
  readonly property real labelPixelSize: Math.round(44 * scaleHint)

  readonly property color keyColor: "#1fffffff"
  readonly property color functionKeyColor: "#0fffffff"
  readonly property color pressedKeyColor: "#47ffffff"
  readonly property color latchedKeyColor: "#33ffffff"
  readonly property color lockedKeyColor: "#5cffffff"
  readonly property color textColor: "#ffffff"
  readonly property color secondaryTextColor: "#b2ffffff"
  readonly property color popupColor: "#f2303030"
  readonly property color popupBorderColor: "#1affffff"

  keyboardDesignWidth: 2560
  keyboardDesignHeight: 800
  keyboardRelativeLeftMargin: 6 / keyboardDesignWidth
  keyboardRelativeRightMargin: 6 / keyboardDesignWidth
  keyboardRelativeTopMargin: 6 / keyboardDesignHeight
  keyboardRelativeBottomMargin: 6 / keyboardDesignHeight

  keyboardBackground: Item {}

  keyPanel: KeyPanel {
    KeyFace {
      key: control

      KeyLabel {
        text: control.displayText
        font.pixelSize: control.displayText.length > 1
          ? currentStyle.labelPixelSize : currentStyle.letterPixelSize
        font.capitalization: control.uppercased
          ? Font.AllUppercase : Font.MixedCase
      }
    }
  }

  backspaceKeyPanel: KeyPanel {
    KeyFace {
      key: control
      functionKey: true

      KeyLabel {
        text: qsTr("删除")
      }
    }
  }

  enterKeyPanel: KeyPanel {
    KeyFace {
      key: control
      functionKey: true

      KeyLabel {
        text: qsTr("确定")
      }
    }
  }

  shiftKeyPanel: KeyPanel {
    KeyFace {
      key: control
      functionKey: true
      latched: InputContext.shiftActive
      locked: InputContext.capsLockActive

      Image {
        anchors.centerIn: parent
        sourceSize.height: Math.round(64 * currentStyle.scaleHint)
        source: currentStyle.resourcePrefix + (InputContext.shiftActive
          ? "images/shift-active.svg" : "images/shift.svg")
        opacity: control.enabled ? 1.0 : 0.3
      }
    }
  }

  spaceKeyPanel: KeyPanel {
    KeyFace {
      key: control

      KeyLabel {
        text: qsTr("空格")
        color: currentStyle.secondaryTextColor
      }
    }
  }

  symbolKeyPanel: KeyPanel {
    KeyFace {
      key: control
      functionKey: true

      KeyLabel {
        text: control.displayText
      }
    }
  }

  characterPreviewMargin: 0
  characterPreviewDelegate: Item {
    property string text

    Rectangle {
      anchors.fill: parent
      radius: currentStyle.keyRadius
      color: currentStyle.popupColor
      border.width: 1
      border.color: currentStyle.popupBorderColor

      Text {
        anchors.fill: parent
        text: parent.parent.text
        color: currentStyle.textColor
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        font.family: currentStyle.fontFamily
        font.pixelSize: Math.round(82 * currentStyle.scaleHint)
      }
    }
  }

  navigationHighlight: Rectangle {
    color: "transparent"
    radius: currentStyle.keyRadius
    border.width: 2
    border.color: "#99ffffff"
  }

  selectionHandle: Rectangle {
    width: 16
    height: 16
    radius: 8
    color: currentStyle.textColor
  }

  component KeyFace: Rectangle {
    required property var key
    property bool functionKey: false
    property bool latched: false
    property bool locked: false

    anchors.fill: parent
    anchors.margins: currentStyle.keyMargin
    radius: currentStyle.keyRadius
    opacity: key.enabled ? 1.0 : 0.5
    color: key.pressed ? currentStyle.pressedKeyColor
      : locked ? currentStyle.lockedKeyColor
      : latched ? currentStyle.latchedKeyColor
      : functionKey ? currentStyle.functionKeyColor
      : currentStyle.keyColor

    Behavior on color {
      ColorAnimation {
        duration: 80
      }
    }
  }

  component KeyLabel: Text {
    anchors.fill: parent
    color: currentStyle.textColor
    horizontalAlignment: Text.AlignHCenter
    verticalAlignment: Text.AlignVCenter
    elide: Text.ElideRight
    font.family: currentStyle.fontFamily
    font.pixelSize: currentStyle.labelPixelSize
  }
}
