# Session bus: the lockers own top.gxde.Lock and are activated on demand.
configure_file(
    "top.gxde.Lock.service.in"
    "${CMAKE_CURRENT_BINARY_DIR}/session/top.gxde.Lock.service"
    @ONLY
)
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/session/top.gxde.Lock.service"
    DESTINATION "${CMAKE_INSTALL_FULL_DATADIR}/dbus-1/services"
)

# System bus: the root helper owns top.gxde.Lock and is activated on demand.
configure_file(
    "top.gxde.Lock.System.service.in"
    "${CMAKE_CURRENT_BINARY_DIR}/system/top.gxde.Lock.service"
    @ONLY
)
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/system/top.gxde.Lock.service"
    DESTINATION "${CMAKE_INSTALL_FULL_DATADIR}/dbus-1/system-services"
)
install(FILES
    "top.gxde.Lock.conf"
    DESTINATION "${DBUS_SYSTEM_CONFIG_DIR}"
)

configure_file(
    "gxde-lock.desktop.in"
    "${CMAKE_CURRENT_BINARY_DIR}/gxde-lock.desktop"
    @ONLY
)
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/gxde-lock.desktop"
    DESTINATION "${CMAKE_INSTALL_SYSCONFDIR}/xdg/autostart"
)

# Oh-My-GXDE integration; third-party packaging relies on this path.
install(FILES
    "configs/OMG.conf"
    DESTINATION "${CMAKE_INSTALL_SYSCONFDIR}/gxde-lock"
)
