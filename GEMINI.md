# GEMINI.md

Project-specific guidance for AI agents working in the gxde-lock repository.
Read this together with `README.md`, `INSTALL.md`, and the files under `docs/`.

## 1. Project Overview

gxde-lock is the in-session screen locker of GXDE OS. It was split out of the
GXDE Display Manager (`GXDM`, an SDDM fork); the display manager itself (daemon,
greeters, session helpers, themes) is not part of this repository.

| Component | Binary / target | Source | Purpose |
| --- | --- | --- | --- |
| Classic lock screen | `gxde-lock-classic` | `src/lock-classic` | DTK2Widget lock screen inherited from DDE `session-ui`. Stable entry point (run by the guard); execs `gxde-lock-neo` for the modern style. |
| Guard | `gxde-lock-guard` (libexec) | `src/guard` | Autostart/D-Bus entry point. Supervises the locker, restarts it locked after a crash, shows the crash notice (`--notice`) after repeated crashes. |
| Modern lock screen | `gxde-lock-neo` | `src/lock-neo` | Qt Quick lock screen backed by `src/lock-common`. |
| Shared lock backend | `gxde-lock-backend`, `gxde-lock-common` (static) | `src/lock-common`, `src/lock-classic/lock` | PAM worker, fingerprint unlock, Wayland session lock, D-Bus surface, style resolution, media/volume/notifications. |
| Auth helper | `gxde-lock-auth-helper` (libexec) | `src/auth-helper` | Runs the `gxde-lock` PAM stack as the session user; authentication only. |
| Root helper | `gxde-lock-system` (libexec) | `src/system` | System-bus service for settings shared by all users and power actions; D-Bus activated, exits when idle. |

Key facts:

- **Languages:** C++ (C++20 under CMake; `src/lock-neo` uses C++17; Bazel
  defaults to C++17 with per-target C++20), QML, CMake, Bazel, shell.
- **UI stack:** Qt 6.5+, Qt Quick/QML, DTK2Widget/DTK6, LayerShellQt,
  GSettings-Qt6.
- **Display servers:** both lock screens support X11 (full-screen windows with
  keyboard grabs) and Wayland (`ext-session-lock-v1`, falling back to
  LayerShellQt).
- **Upstream heritage:** `src/auth`, `src/auth-helper` and parts of
  `src/common` come from SDDM and keep `namespace SDDM` and their GPL-2.0+
  headers. `src/lock-classic/vendor` is vendored code (`namespace gxdm`); do
  not modify it in place.
- **License:** GPL-3.0-or-later for GXDE code. Preserve existing headers and
  add a GPL-3.0-or-later header to new files.

## 2. Repository Map

| Path | Contents |
| --- | --- |
| `src/auth` | `SDDM::Auth` client: spawns the auth helper and speaks its socket protocol. |
| `src/auth-helper` | `gxde-lock-auth-helper`: `HelperApp`, `PamBackend`, `PamHandle`. |
| `src/common` | `Constants.h.in`, `SafeDataStream`, `MessageHandler`, `LockAppearance` (wallpaper/cursor state), `LockCursor`, `LockScale`, `SystemDefaults`, `FingerprintProbe`, `FingerprintSession`, `LogindSession` (logind Lock/Unlock, LockedHint), `LockState` (runtime lock flag read by the guard), `LockTranslations` (installs the gxde-lock, qtbase and legacy gxde-session-ui translators). |
| `src/guard` | `gxde-lock-guard`: `LockGuard` (supervisor), `BrokenNotice` and `NoticeWindow` (crash notice). |
| `src/lock-classic` | DTK classic lock screen. `lock/` holds the locker and backend pieces (`LockWorker`, `LockShortcutManager`/`GxdeLockService`, `WaylandSessionLockIntegration`, `FingerprintUnlock`); `widgets/`, `session_widgets/`, `global_util/` hold the DDE widgets. |
| `src/lock-common` | QML-facing facade (`LockBackend`, `UserListModel`), `LockStyleResolver`, `LockStyleManager`, media/volume/notification helpers and their tests. |
| `src/lock-neo` | `gxde-lock-neo`: `src/main.cc`, `qml/`, `icons/`, `res/` (power/keyboard icons and the virtual keyboard style). |
| `src/system` | `gxde-lock-system`: `LockSystemService`, `SaveGeneralSetting`. |
| `data` | D-Bus activation and policy files, `interfaces/top.gxde.Lock.System.xml`, autostart entry, `configs/OMG.conf`. |
| `services` | PAM stacks (generic, Debian, FreeBSD). |
| `translations` | `gxde-lock_<lang>.ts` catalogs (ar, en, es, fr, ru, zh_CN; English sources). Refresh with the `gxde-lock-lupdate` CMake target. |
| `test` | QtTest suite for appearance, scale, system defaults, virtual keyboard, multi-screen, settings persistence and the auth protocol. |
| `debian`, `arch` | Distribution packaging. |

## 3. Runtime Architecture

- The XDG autostart entry (and session-bus activation) runs
  `gxde-lock-guard --daemon --auto-enroll`, which runs `gxde-lock-classic`
  with the same arguments.
  The locker owns `top.gxde.Lock` (session bus), `org.freedesktop.ScreenSaver`
  and `com.deepin.dde.lockFront`, and registers Super+L via KGlobalAccel
  without stealing an existing binding.
- When `[General] LockScreenStyle` in `/etc/gxde-lock.conf` (or
  `[NextFeatReq] SwitchLkScrStyle` in `/etc/gxde-lock/OMG.conf`) resolves to
  `modern` (the default), the classic binary execs `gxde-lock-neo` with the same
  arguments. A missing neo binary means classic.
- Unlock: `SDDM::Auth` starts `gxde-lock-auth-helper` as the session user. The
  helper runs PAM service `gxde-lock` (or `gxde-lock-fingerprint`), reports the
  result over a local socket, and exits. It refuses `--start` and
  `--display-server`.
- Shared settings: `GxdeLockService` forwards `SetLockScreenStyle`,
  `SetWallpaper*`, `ClearWallpaper` and `SetCursor` to `gxde-lock-system`
  (`top.gxde.Lock` on the system bus, interface `top.gxde.Lock.System`). It
  writes `/etc/gxde-lock.conf` and `/var/lib/gxde-lock/state.conf`
  (`[Lock] Wallpaper`, `[Lock] CursorTheme`) and emits
  `LockScreenStyleChanged`/`WallpaperChanged`. Wallpapers arrive as file
  descriptors, never as caller-supplied paths.
- Per-user lock wallpaper overrides stay in
  `~/.local/share/gxde-lock/lock-wallpaper-override` and never go through root.
- Crash recovery: the lockers keep `$XDG_RUNTIME_DIR/gxde-lock/locked-<display>`
  while locked. When the locker dies, the guard restarts it without
  `--daemon` if that flag exists (a new ext-session-lock client replaces the
  abandoned lock), otherwise with `--daemon`. Three crashes within 60 s while
  locked switch to `gxde-lock-guard --notice`, which holds the lock and
  explains `loginctl unlock-session`; it exits after logind `Unlock` or
  SIGUSR1, and the guard restarts the daemon.
- A style change while locked is deferred until unlock; the locker then
  releases its D-Bus names (and neo's instance lock) and execs the other binary
  with `--daemon`.

Paths come from `src/common/Constants.h.in` (`STATE_DIR`, `CONFIG_FILE`,
`OMG_CONFIG_FILE`, `BIN_INSTALL_DIR`, `LIBEXEC_INSTALL_DIR`); do not hardcode
them in new code.

## 4. Building

```sh
cmake -S . -B build-cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr
cmake --build build-cmake -j4
DESTDIR="$PWD/build-cmake/stage" cmake --install build-cmake
```

Keep build parallelism low (`-j4`): the DTK/Qt translation units are large.

| Option | Default | Effect |
| --- | --- | --- |
| `BUILD_NEO_LOCK` | `ON` | Build `gxde-lock-neo`. |
| `LOCK_BUILD_TESTS` | `OFF` | Build the `src/lock-common/tests` CTest cases. |
| `INSTALL_PAM_CONFIGURATION` | `ON` | Install the platform PAM files. |
| `ENABLE_JOURNALD` | `ON` | journald logging for the auth helper. |
| `NO_SYSTEMD` / `USE_ELOGIND` | `OFF` | Non-systemd builds. |

The root build adds `-Wall -Wextra` and the Qt porting guards
`QT_NO_CAST_FROM_ASCII`, `QT_NO_CAST_FROM_BYTEARRAY` and `QT_NO_FOREACH`;
`src/lock-classic` removes them for legacy DTK code.

Bazel (`bazel build --spawn_strategy=local //:gxde-lock-dist`) uses host Qt and
DTK at Debian paths from `MODULE.bazel`. When you add a source file, update both
the `CMakeLists.txt` and the `BUILD.bazel` that own it.

Packaging: `./build-deb -d` (Debian, package `gxde-lock`), `cd arch && makepkg
-si` (Arch, `gxde-lock-git`). `./update-pkgver` syncs `arch/PKGBUILD` from
`debian/changelog`.

## 5. Testing

```sh
cmake -S . -B build-cmake -DLOCK_BUILD_TESTS=ON
cmake --build build-cmake -j4
QT_QPA_PLATFORM=offscreen ctest --test-dir build-cmake --output-on-failure
```

Test inventory: `SystemDefaults`, `LockAppearance`, `LockScale`,
`VirtualKeyboard`, `MultiScreenManager`, `GeneralSetting`, `AuthTransition`
(`test/`), plus `LockStyle`, `LockBackend`, `LockMedia`, `LockServices`,
`LockFingerprint` (`src/lock-common/tests`, with `LOCK_BUILD_TESTS=ON`).

GUI/QML tests run offscreen. Never test real locking on a daily-use desktop:
use a VM or a nested/headless compositor. `gxde-lock-neo --test-mode` previews
the UI without locking (any password works, `Esc` exits).

## 6. Development Conventions

- New C++ follows the Google C++ Style Guide (2-space indent, `.cc`, one class
  per `.cc`/`.h`), as in `src/system` and `src/lock-common`. Older SDDM and DDE
  files keep their own style; match the surrounding file.
- Prefer `QStringLiteral`/`QLatin1String` and range-based `for`.
- GXDE code uses `namespace gxde::lock` (or `gxde`); SDDM code stays in
  `namespace SDDM`.
- Writing configuration must preserve unrelated keys and comments
  (`gxde::SaveGeneralSetting`) and must be atomic.
- Anti-lockout: unlocking requires PAM success, or logind's `Unlock` for the
  session (`loginctl unlock-session`, which logind only accepts from the
  session owner or an administrator). Never add an unlock shortcut. Every
  crash path must end in a running locker or the crash notice, never an
  abandoned lock. PAM stacks must not contain
  checks that can refuse a user who is already logged in (`pam_nologin`,
  `pam_shells`, `user != root`).
- When changing the D-Bus API, update `README.md`/`README.zh.md`,
  `data/interfaces/top.gxde.Lock.System.xml` for the system side, and the D-Bus
  policy if access changes.
- Do not bump versions or commit unless asked. Commit subjects use
  Conventional Commit prefixes (`feat:`, `fix:`, `bump:`).

Do not edit or commit build output (`build*/`, `bazel-*`), generated
`Constants.h`, `*.moc`, `moc_*.cpp`, `qrc_*.cpp`, `compile_commands.json` or
packaged `.deb`/`.tar` artifacts.
