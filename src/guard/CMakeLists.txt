# gxde-lock-guard: started by the autostart entry and D-Bus activation in
# place of gxde-lock-classic. It restarts a crashed locker (locked again when
# the screen was locked) and, when the locker keeps crashing, holds the lock
# with a notice explaining how to unlock from a text console.

find_package(Qt6 COMPONENTS Core Gui DBus WaylandClient REQUIRED)
if(NOT TARGET Qt6::WaylandClientPrivate)
    find_package(Qt6 COMPONENTS WaylandClientPrivate REQUIRED)
endif()

add_executable(gxde-lock-guard
    main.cc
    lock_guard.cc
    lock_guard.h
    broken_notice.cc
    broken_notice.h
    notice_window.cc
    notice_window.h
    "${CMAKE_SOURCE_DIR}/src/common/LockState.cpp"
    "${CMAKE_SOURCE_DIR}/src/common/LockTranslations.cpp"
    "${CMAKE_SOURCE_DIR}/src/common/LockTranslations.h"
    "${CMAKE_SOURCE_DIR}/src/common/LockState.h"
    "${CMAKE_SOURCE_DIR}/src/common/LogindSession.cpp"
    "${CMAKE_SOURCE_DIR}/src/common/LogindSession.h"
    "${CMAKE_SOURCE_DIR}/src/lock-classic/lock/waylandsessionlock.cpp"
    "${CMAKE_SOURCE_DIR}/src/lock-classic/lock/waylandsessionlock.h"
)
target_include_directories(gxde-lock-guard PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_SOURCE_DIR}/src/common"
    "${CMAKE_SOURCE_DIR}/src/lock-classic/lock"
    "${CMAKE_BINARY_DIR}/src/common"
)
target_link_libraries(gxde-lock-guard PRIVATE
    gxde-lock-ext-session-lock-protocol
    Qt6::Gui
    Qt6::DBus
    Qt6::WaylandClientPrivate
    X11
)
install(TARGETS gxde-lock-guard RUNTIME DESTINATION "${CMAKE_INSTALL_LIBEXECDIR}")
