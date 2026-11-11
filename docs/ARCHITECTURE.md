## Introduction
gxde-lock locks a running desktop session. It never starts sessions; it
consists of two lock screen frontends, a guard that keeps one of them running,
and two small helpers.

## Lock screens
`gxde-lock-classic` (DTK widgets) and `gxde-lock-neo` (Qt Quick) run as the
session user. The classic binary is the stable entry point: `gxde-lock-guard`
(see below) starts it, and it execs
`gxde-lock-neo` with the same arguments when the configured style is
`modern`. Both share the backend in `src/lock-common` and
`src/lock-classic/lock`:

- `LockWorker` drives authentication through `SDDM::Auth`;
- `FingerprintUnlock` runs fingerprint attempts next to the password;
- `WaylandSessionLockIntegration` implements `ext-session-lock-v1`, with LayerShellQt as
  the fallback; on X11 the windows grab the keyboard;
- `GxdeLockService` is the public session-bus API (`top.gxde.Lock`);
- `LogindSession` follows `loginctl lock-session`/`unlock-session` and sets
  the session's `LockedHint`;
- `LockStyleManager` re-reads the style and requests a handoff after unlock.

## Guard
`gxde-lock-guard` is what the autostart entry and the session-bus activation
file start. It runs `gxde-lock-classic` and waits for it. If the locker dies
while the screen is locked (the lockers keep a flag in `$XDG_RUNTIME_DIR`
while locked), the guard starts a new one that locks at once; on Wayland the
new ext-session-lock client replaces the abandoned lock. After three crashes
in a minute it runs itself with `--notice`: a raster-only lock surface that
explains how to unlock from a text console with `loginctl unlock-session`
(or SIGUSR1) and then hands control back to a fresh locker.

## Authentication
`SDDM::Auth` starts `gxde-lock-auth-helper` as the session user and talks to it
over a private local socket. The helper runs the `gxde-lock` (or
`gxde-lock-fingerprint`) PAM stack in authentication-only mode and exits;
`pam_unix` checks the password through the setuid `unix_chkpwd`.

## Shared settings
Settings that apply to every user need root. `gxde-lock-system` is activated
on the system bus when a locker forwards such a call, saves the lock screen
style to `/etc/gxde-lock.conf` and the global wallpaper and cursor theme to
`/var/lib/gxde-lock/state.conf`, performs power actions through logind, and
exits when idle.
