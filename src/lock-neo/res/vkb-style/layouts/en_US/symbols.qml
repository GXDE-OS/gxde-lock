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
import QtQuick.Layouts
import QtQuick.VirtualKeyboard
import QtQuick.VirtualKeyboard.Components

KeyboardLayoutLoader {
  property bool secondPage

  onVisibleChanged: if (!visible) secondPage = false
  sourceComponent: secondPage ? page2 : page1

  Component {
    id: page1

    KeyboardLayout {
      keyWeight: 160
      readonly property real normalKeyWidth: normalKey.width
      readonly property real functionKeyWidth:
        mapFromItem(normalKey, normalKey.width / 2, 0).x

      KeyboardRow {
        Key {
          key: Qt.Key_1
          text: "1"
        }
        Key {
          id: normalKey
          key: Qt.Key_2
          text: "2"
        }
        Key {
          key: Qt.Key_3
          text: "3"
        }
        Key {
          key: Qt.Key_4
          text: "4"
        }
        Key {
          key: Qt.Key_5
          text: "5"
        }
        Key {
          key: Qt.Key_6
          text: "6"
        }
        Key {
          key: Qt.Key_7
          text: "7"
        }
        Key {
          key: Qt.Key_8
          text: "8"
        }
        Key {
          key: Qt.Key_9
          text: "9"
        }
        Key {
          key: Qt.Key_0
          text: "0"
        }
      }
      KeyboardRow {
        Key {
          key: Qt.Key_At
          text: "@"
        }
        Key {
          key: Qt.Key_NumberSign
          text: "#"
        }
        Key {
          key: Qt.Key_Dollar
          text: "$"
        }
        Key {
          key: Qt.Key_Percent
          text: "%"
        }
        Key {
          key: Qt.Key_Ampersand
          text: "&"
        }
        Key {
          key: Qt.Key_Asterisk
          text: "*"
        }
        Key {
          key: Qt.Key_Minus
          text: "-"
        }
        Key {
          key: Qt.Key_Plus
          text: "+"
        }
        Key {
          key: Qt.Key_ParenLeft
          text: "("
        }
        Key {
          key: Qt.Key_ParenRight
          text: ")"
        }
      }
      KeyboardRow {
        Key {
          displayText: "1/2"
          functionKey: true
          highlighted: true
          weight: functionKeyWidth
          Layout.fillWidth: false
          onClicked: secondPage = !secondPage
        }
        Key {
          key: Qt.Key_Exclam
          text: "!"
        }
        Key {
          key: Qt.Key_QuoteDbl
          text: "\""
        }
        Key {
          key: Qt.Key_Apostrophe
          text: "'"
        }
        Key {
          key: Qt.Key_Colon
          text: ":"
        }
        Key {
          key: Qt.Key_Semicolon
          text: ";"
        }
        Key {
          key: Qt.Key_Slash
          text: "/"
        }
        Key {
          key: Qt.Key_Question
          text: "?"
        }
        BackspaceKey {
          weight: functionKeyWidth
          Layout.fillWidth: false
        }
      }
      KeyboardRow {
        SymbolModeKey {
          weight: functionKeyWidth
          Layout.fillWidth: false
          displayText: "ABC"
        }
        SpaceKey {}
        EnterKey {
          weight: functionKeyWidth
          Layout.fillWidth: false
        }
      }
    }
  }

  Component {
    id: page2

    KeyboardLayout {
      keyWeight: 160
      readonly property real normalKeyWidth: normalKey.width
      readonly property real functionKeyWidth:
        mapFromItem(normalKey, normalKey.width / 2, 0).x

      KeyboardRow {
        Key {
          key: Qt.Key_1
          text: "1"
        }
        Key {
          id: normalKey
          key: Qt.Key_2
          text: "2"
        }
        Key {
          key: Qt.Key_3
          text: "3"
        }
        Key {
          key: Qt.Key_4
          text: "4"
        }
        Key {
          key: Qt.Key_5
          text: "5"
        }
        Key {
          key: Qt.Key_6
          text: "6"
        }
        Key {
          key: Qt.Key_7
          text: "7"
        }
        Key {
          key: Qt.Key_8
          text: "8"
        }
        Key {
          key: Qt.Key_9
          text: "9"
        }
        Key {
          key: Qt.Key_0
          text: "0"
        }
      }
      KeyboardRow {
        Key {
          key: Qt.Key_AsciiTilde
          text: "~"
        }
        Key {
          key: Qt.Key_QuoteLeft
          text: "`"
        }
        Key {
          key: Qt.Key_Bar
          text: "|"
        }
        Key {
          key: Qt.Key_Backslash
          text: "\\"
        }
        Key {
          key: Qt.Key_BraceLeft
          text: "{"
        }
        Key {
          key: Qt.Key_BraceRight
          text: "}"
        }
        Key {
          key: Qt.Key_BracketLeft
          text: "["
        }
        Key {
          key: Qt.Key_BracketRight
          text: "]"
        }
        Key {
          key: Qt.Key_AsciiCircum
          text: "^"
        }
        Key {
          key: Qt.Key_Equal
          text: "="
        }
      }
      KeyboardRow {
        Key {
          displayText: "2/2"
          functionKey: true
          highlighted: true
          weight: functionKeyWidth
          Layout.fillWidth: false
          onClicked: secondPage = !secondPage
        }
        FillerKey {
          weight: normalKeyWidth * 2 / 2
          Layout.fillWidth: false
        }
        Key {
          key: Qt.Key_Underscore
          text: "_"
        }
        Key {
          key: Qt.Key_Less
          text: "<"
        }
        Key {
          key: Qt.Key_Greater
          text: ">"
        }
        Key {
          key: Qt.Key_Comma
          text: ","
        }
        Key {
          key: Qt.Key_Period
          text: "."
        }
        FillerKey {
          weight: normalKeyWidth * 2 / 2
          Layout.fillWidth: false
        }
        BackspaceKey {
          weight: functionKeyWidth
          Layout.fillWidth: false
        }
      }
      KeyboardRow {
        SymbolModeKey {
          weight: functionKeyWidth
          Layout.fillWidth: false
          displayText: "ABC"
        }
        SpaceKey {}
        EnterKey {
          weight: functionKeyWidth
          Layout.fillWidth: false
        }
      }
    }
  }
}
