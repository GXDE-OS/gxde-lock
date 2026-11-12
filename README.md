![HEADER](./docs/img/header.png)

![CONTRIBUTORS](https://img.shields.io/github/contributors/GXDE-OS/gxde-lock.svg?style=plastic)
![ISSUES](https://img.shields.io/github/issues/GXDE-OS/gxde-lock.svg?style=plastic)
![LICENSE](https://img.shields.io/badge/license-GPLv3-blue)
![](https://img.shields.io/badge/made_with-love-red)


<br />
<div align="center">
  <h3 align="center">GXDE Lock</h3>

  <p align="center">
    The in-session lock screen of GXDE OS, for X11 and Wayland.
    <br />
    <a href="https://gitee.com/GXDE-OS/gxde-lock/issues">Fire up an issue</a>
    &middot;
    <a href="https://gitee.com/GXDE-OS/gxde-lock/issues">Request features</a>
  </p>
</div>

[中文](./README.zh.md)

## About the Project
`gxde-lock` is the screen locker of the [GXDE Display Manager](https://github.com/GXDE-OS/gxde-display-manager)
(`GXDM`, an SDDM fork), split out on its own. The display manager parts (daemon,
greeters, session helpers, themes) are gone; what is left is everything needed
to lock and unlock a running session:

| Component | Installed as | Source | Purpose |
| --- | --- | --- | --- |
| Classic lock screen | `/usr/bin/gxde-lock-classic` | `src/lock-classic` | DTK widget lock screen. Also the stable entry point behind the guard; hands off to the modern lock screen when that style is selected. |
| Modern lock screen | `/usr/bin/gxde-lock-neo` | `src/lock-neo` | Qt Quick lock screen backed by `src/lock-common`. |
| Guard | `<libexec>/gxde-lock-guard` | `src/guard` | Started by autostart and D-Bus activation. Runs the classic lock screen, restarts it if it crashes, and holds the lock with a crash notice when it keeps crashing. |
| Auth helper | `<libexec>/gxde-lock-auth-helper` | `src/auth-helper` | Runs the `gxde-lock` PAM stack as the locked user; never opens a session. |
| Root helper | `<libexec>/gxde-lock-system` | `src/system` | System-bus service, activated on demand, that stores the settings shared by all users and performs power actions. |

Both lock screens run on X11 (full-screen windows with keyboard grabs) and on
Wayland (`ext-session-lock-v1` when the compositor supports it, LayerShellQt
`zwlr_layer_shell_v1` otherwise). Both unlock with a password or, when
`fprintd` has a reader and enrolled fingers, a fingerprint.

### Toolchains
#### Build Dependencies
* CMake 3.21+ (or Bazel/Bzlmod on Debian hosts) and a C++20 compiler
* PAM, X11, Xcursor, Xfixes, XInput, XRandR, XTest, Xext
* **Qt 6.5+**: Core, DBus, Gui, Widgets, Network, Qml, Quick, Svg, Xml, WaylandClient
* **DTK**: DTK2Widget-Qt6, DTK6 Core, DTK6 DLog
* gsettings-qt6, LayerShellQt, wayland-protocols, wayland-scanner

***NOTE**: [`debian/control`](./debian/control) has the full Debian list.*

#### Runtime
* PAM, logind (or elogind), D-Bus
* AccountsService
* An X11 session, or a Wayland compositor with `ext-session-lock-v1` or `zwlr_layer_shell_v1`
* Qt Quick, Qt Quick Controls/Layouts/Effects and Qt Virtual Keyboard QML modules for `gxde-lock-neo`

## Getting Started
### Building From Source (CMake)
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr
cmake --build build -j4
DESTDIR="$PWD/build/stage" cmake --install build   # inspect first
sudo cmake --install build
```

| Option | Default | Effect |
| --- | --- | --- |
| `BUILD_NEO_LOCK` | `ON` | Build `gxde-lock-neo`. |
| `LOCK_BUILD_TESTS` | `OFF` | Build the lock backend tests (`LockStyle`, `LockBackend`, `LockMedia`, `LockServices`, `LockFingerprint`). |
| `INSTALL_PAM_CONFIGURATION` | `ON` | Install `/etc/pam.d/gxde-lock` and `gxde-lock-fingerprint`. |
| `ENABLE_JOURNALD` | `ON` | Log the auth helper to journald. |
| `NO_SYSTEMD` / `USE_ELOGIND` | `OFF` | Build without systemd, or against elogind. |

### Building From Source (Bazel)
```bash
bazel build --spawn_strategy=local //:gxde-lock-dist
```
The Bazel build uses the host Qt and DTK at Debian paths (see `MODULE.bazel`).

### Packages
```bash
./build-deb -d          # Debian / GXDE OS
cd arch && makepkg -si  # Arch Linux
```

## Usage
The package installs an XDG autostart entry, so the locker starts with the
session (`gxde-lock-guard --daemon --auto-enroll`, which runs
`gxde-lock-classic` with those arguments). It also registers:

* `org.freedesktop.ScreenSaver` and the Deepin `com.deepin.dde.lockFront`
  names, so desktop "Lock Screen" actions reach it;
* the Super+L shortcut through KGlobalAccel, unless something else owns it;
* the logind session's `Lock`/`Unlock` signals, so `loginctl lock-session` and
  `loginctl unlock-session` work, and the session's `LockedHint`.

### When the lock screen crashes
If the locker dies while the screen is locked, `gxde-lock-guard` starts a new
one that takes the lock over at once (on Wayland the compositor keeps the
session locked in the meantime). After three crashes within a minute it shows
a plain notice instead, like Plasma does: switch to a text console, log in and
run the `loginctl unlock-session <id>` (or `kill -USR1 <pid>`) command it
displays, then switch back. There is deliberately no unlock shortcut.

#### Compositor requirements
On Wayland, recovery depends on the compositor:

* **`ext-session-lock-v1`:** when a lock client dies, the compositor keeps
  the session locked. The compositor must then let a new client lock the
  session and take over the abandoned lock; otherwise neither the restarted
  locker nor the crash notice can put anything on screen, and the user only
  sees the compositor's own fallback (often a black screen). sway 1.10 does
  this (verified; the log shows `Replacing abandoned lock`). If your
  compositor cannot, it should at least draw its own message for an
  abandoned lock, as KWin does on Plasma.
* **`zwlr_layer_shell_v1` fallback:** the lock surfaces disappear together
  with the crashed locker, so the session is briefly unlocked until
  `gxde-lock-guard` has locked it again.

X11 needs nothing from the window manager: the restarted locker and the
notice use override-redirect windows and grab the keyboard and pointer.

Lock right away:
```bash
gxde-lock-classic                # or: gxde-lock-neo
```

Preview without locking (any password works, `Esc` quits):
```bash
gxde-lock-neo --test-mode
gxde-lock-neo --test-mode --switch
```

### Lock screen style
`[General] LockScreenStyle` in `/etc/gxde-lock.conf` picks the lock screen:

```ini
[General]
LockScreenStyle=modern
```

`modern` (the default) runs `gxde-lock-neo`, `classic` runs `gxde-lock-classic`.
When the key is absent, `[NextFeatReq] SwitchLkScrStyle` in
`/etc/gxde-lock/OMG.conf` is used. A missing `gxde-lock-neo` always means
classic. Switching at runtime through `SetLockScreenStyle` takes effect after
the next unlock: the running locker releases its D-Bus names and execs the
other one, so the session is never left unlocked.

### D-Bus API
Applications use the session bus:

| Item | Value |
| --- | --- |
| Bus | Session bus |
| Service | `top.gxde.Lock` |
| Object path | `/top/gxde/Lock` |
| Interface | `top.gxde.Lock` |

| Method | Description |
| --- | --- |
| `TryEnrollLkScr(bool enabled) -> bool` | Registers or withdraws Super+L. An existing Super+L binding is never stolen. |
| `UnregisterLkScr()` | Withdraws Super+L and stops automatic re-enrollment for this locker process. |
| `LkScrStat() -> bool` | Whether gxde-lock owns Super+L. |
| `Show()` | Locks the screen. |
| `SetCursor(string theme) -> bool` | Sets the lock screen cursor theme for all users. The theme must be installed. |
| `SetWallpaperGXDEDefault() -> bool` | Uses the GXDE default as the global lock wallpaper. |
| `SetWallpaperDDELockDefault() -> bool` | Uses the bundled DDE lock background as the global lock wallpaper. |
| `SetWallpaper(string path) -> bool` | Sets a custom global lock wallpaper from a local path or `file://` URL. |
| `ClearWallpaper() -> bool` | Removes the global lock wallpaper. |
| `SetLockWallpaperOverride(string path) -> bool` | Sets the current user's own lock wallpaper. |
| `ClearLockWallpaperOverride() -> bool` | Clears the current user's lock wallpaper. |
| `LockWallpaperOverride() -> string` | The current user's lock wallpaper, or an empty string. |
| `SetLockScreenStyle(string style) -> bool` | Persists `classic` or `modern` for all users. Fails when that lock screen is not installed. |
| `LockScreenStyle() -> string` | The effective lock screen style. |

Signals: `LkScrStatChanged(bool)`, `LockScreenStyleChanged(string)`.

Examples:
```bash
busctl --user call top.gxde.Lock /top/gxde/Lock top.gxde.Lock TryEnrollLkScr b true
busctl --user call top.gxde.Lock /top/gxde/Lock top.gxde.Lock Show
busctl --user call top.gxde.Lock /top/gxde/Lock top.gxde.Lock SetWallpaper s /path/to/image.jpg
busctl --user call top.gxde.Lock /top/gxde/Lock top.gxde.Lock SetLockWallpaperOverride s /path/to/image.jpg
busctl --user call top.gxde.Lock /top/gxde/Lock top.gxde.Lock SetLockScreenStyle s classic
```

Settings shared by every user (style, global wallpaper, cursor theme) need
root, so the session interface forwards them to `gxde-lock-system`, which owns
`top.gxde.Lock` on the **system** bus with the internal
`top.gxde.Lock.System` interface. It writes `/etc/gxde-lock.conf` and
`/var/lib/gxde-lock/state.conf`, copies custom wallpapers (passed as file
descriptors, at most 128 MiB) into `/var/lib/gxde-lock`, and exits after a
minute of inactivity. Do not call it directly.

The wallpaper of a user's lock screen is, in order: the user's
`LockWallpaperOverride` (`~/.local/share/gxde-lock/lock-wallpaper-override`),
Deepin Accounts `GreeterBackground`/`BackgroundFile`/`DesktopBackgrounds`,
AccountsService, the GSettings desktop background, then the global lock
wallpaper or the GXDE default.

### Fingerprint unlock
See [docs/FINGERPRINT.md](./docs/FINGERPRINT.md) (Chinese).

## Contributing
Issues and pull requests are welcome.

### Translations
Both lock screens and the crash notice use Qt's translation system
(`tr()` / `qsTr()`, English source strings). The catalogs live in
`translations/gxde-lock_<language>.ts`. The project ships Arabic, Chinese,
English, French, Russian and Spanish, the six official UN languages. They are
installed as `.qm` files under `/usr/share/gxde-lock/translations`.

The lock screen follows `LANGUAGE`/`LANG`. The classic style switches to the
language of the user it locks for. Right-to-left languages mirror the layout.

After changing strings, refresh the catalogs with
`cmake --build build --target gxde-lock-lupdate`, then translate the new
entries in Qt Linguist. To add a language, add its code to
`translations/CMakeLists.txt` and run the same target.

### Packaging for Other Distros
Install `data/configs/OMG.conf` to `/etc/gxde-lock/` (CMake does this) and
the PAM files from `services/` as `/etc/pam.d/gxde-lock` and
`/etc/pam.d/gxde-lock-fingerprint`.

***NOTE**: gxde-lock comes from GXDM, which is based on SDDM; SDDM's contributors are listed [here](https://github.com/sddm/sddm/graphs/contributors?all=1).*

## License
This project is licensed under [GNU GENERAL PUBLIC LICENSE Version 3](./LICENSE).
Files inherited from SDDM keep their GPL-2.0-or-later headers.

`src/lock-classic` contains code from GXDE `session-ui`
([GXDE-OS/gxde-session-ui](https://gitee.com/GXDE-OS/gxde-session-ui)),
licensed under GPL-3.0 unless a file header says otherwise.

This project contains source code from [CharOfString/samoyed-greeter](https://gitee.com/CharOfString/samoyed-greeter), licensed under GPL-3.0 unless a file header says otherwise.

The lock screen font is [Maven Pro](https://github.com/googlefonts/mavenproFont), licensed under the [SIL Open Font License 1.1](https://github.com/googlefonts/mavenproFont/blob/main/OFL.txt).

## Acknowledgements
* **SDDM**: https://github.com/sddm/sddm
* **GXDE Display Manager**: https://github.com/GXDE-OS/gxde-display-manager
* **GXDE Session UI**: https://gitee.com/GXDE-OS/gxde-session-ui
* **Samoyed Greeter**: https://gitee.com/CharOfString/samoyed-greeter
* **BEST-Readme-Template**: https://github.com/othneildrew/Best-README-Template
