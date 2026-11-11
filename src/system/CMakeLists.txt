# gxde-lock-system: root helper on the system bus. It persists the settings
# shared by every user's locker (style, global wallpaper, cursor theme) and
# performs power actions for the lock screen.

set(LOCK_SYSTEM_SOURCES
    main.cc
    lock_system_service.cc
    lock_system_service.h
    general_setting.cc
    general_setting.h
    "${CMAKE_SOURCE_DIR}/src/lock-common/lockstyle.cpp"
    "${CMAKE_SOURCE_DIR}/src/lock-common/lockstyle.h"
)

qt_add_dbus_adaptor(LOCK_SYSTEM_SOURCES
    "${CMAKE_SOURCE_DIR}/data/interfaces/top.gxde.Lock.System.xml"
    "lock_system_service.h" gxde::lock::LockSystemService locksystemadaptor)

add_executable(gxde-lock-system ${LOCK_SYSTEM_SOURCES})
target_include_directories(gxde-lock-system PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_BINARY_DIR}"
    "${CMAKE_SOURCE_DIR}/src/lock-common"
    "${CMAKE_BINARY_DIR}/src/common"
)
target_link_libraries(gxde-lock-system PRIVATE Qt6::Core Qt6::DBus)
install(TARGETS gxde-lock-system RUNTIME DESTINATION "${CMAKE_INSTALL_LIBEXECDIR}")
