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
import QtQuick.Effects

Rectangle {
  id: avatar

  property url source
  property string name
  property color fallbackColor: "#b9bdc2"
  property color initialsColor: "#4f555b"

  function initialsFor(value) {
    const trimmedName = String(value || "").trim()
    if (trimmedName.length === 0) return "?"
    const words = trimmedName.split(/\s+/)
    if (words.length === 1) return words[0].charAt(0).toUpperCase()
    return (words[0].charAt(0) + words[words.length - 1].charAt(0))
      .toUpperCase()
  }

  radius: 14
  color: fallbackColor
  antialiasing: true

  Text {
    anchors.centerIn: parent
    text: avatar.initialsFor(avatar.name)
    color: avatar.initialsColor
    font.pixelSize: Math.round(Math.min(avatar.width, avatar.height) * 0.34)
    font.weight: Font.Medium
  }

  Rectangle {
    id: avatarMask

    anchors.fill: parent
    radius: avatar.radius
    color: "white"
    antialiasing: true
    visible: false
    layer.enabled: true
    layer.smooth: true
    layer.samples: 4
  }

  Image {
    anchors.fill: parent
    source: avatar.source
    fillMode: Image.PreserveAspectCrop
    asynchronous: true
    cache: true
    smooth: true
    mipmap: true
    // Keep painting the old pixmap while a DPR change reloads it, and never
    // toggle the layer on load status: Qt walks a copy of the child list for
    // DPR updates, and an effect destroyed mid-walk crashes it.
    visible: source.toString() !== "" && status !== Image.Error
    layer.enabled: true
    layer.smooth: true
    layer.effect: MultiEffect {
      maskEnabled: true
      maskSource: avatarMask
      maskThresholdMin: 0.5
      maskSpreadAtMin: 1.0
    }
  }
}
