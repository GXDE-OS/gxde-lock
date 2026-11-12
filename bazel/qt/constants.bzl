"""Install-path constants for gxde-lock
"""

# Multiarch
MULTIARCH = "x86_64-linux-gnu"

CONSTANTS = {
    "CMAKE_INSTALL_FULL_BINDIR": "/usr/bin",
    "CMAKE_INSTALL_FULL_LIBEXECDIR": "/usr/libexec",
    "CMAKE_INSTALL_FULL_SYSCONFDIR": "/etc",
    "CMAKE_INSTALL_FULL_DATADIR": "/usr/share",
    "STATE_DIR": "/var/lib/gxde-lock",
    "CONFIG_FILE": "/etc/gxde-lock.conf",
    "OMG_CONFIG_FILE": "/etc/gxde-lock/OMG.conf",
    "HALT_COMMAND": "/usr/bin/systemctl poweroff",
    "REBOOT_COMMAND": "/usr/bin/systemctl reboot",
}
