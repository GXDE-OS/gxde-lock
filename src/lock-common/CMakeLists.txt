# QML-facing lock facade shared by gxde-lock-neo. This directory is added as a
# subdirectory of src/lock-classic so it inherits the classic locker's
# dependency checks, Qt porting guards and shared source variables.
#
# The legacy widget locker keeps compiling its own copies of these sources; the
# QML locker links the static libraries defined here so both frontends share
# exactly the same authentication, user enumeration and D-Bus behaviour.

set(LOCK_CLASSIC_DIR "${LOCK_REPO_ROOT}/src/lock-classic")

# NEO_VENDOR_SOURCES/NEO_VENDOR_HEADERS are relative to src/lock-classic.
# Make them absolute because this CMakeLists lives in a sibling directory.
set(LOCK_VENDOR_SOURCES)
foreach(source ${NEO_VENDOR_SOURCES})
    list(APPEND LOCK_VENDOR_SOURCES "${LOCK_CLASSIC_DIR}/${source}")
endforeach()
set(LOCK_VENDOR_HEADERS)
foreach(header ${NEO_VENDOR_HEADERS})
    list(APPEND LOCK_VENDOR_HEADERS "${LOCK_CLASSIC_DIR}/${header}")
endforeach()

set(LOCK_BACKEND_SOURCES
    "${LOCK_CLASSIC_DIR}/session_widgets/authinterface.cpp"
    "${LOCK_CLASSIC_DIR}/session_widgets/sessionbasemodel.cpp"
    "${LOCK_CLASSIC_DIR}/session_widgets/userinfo.cpp"
    "${LOCK_CLASSIC_DIR}/lock/fingerprintunlock.cpp"
    "${LOCK_CLASSIC_DIR}/lock/lockworker.cpp"
    "${LOCK_CLASSIC_DIR}/lock/lockshortcutmanager.cpp"
    "${LOCK_CLASSIC_DIR}/lock/dbus/dbuslockagent.cpp"
    "${LOCK_CLASSIC_DIR}/lock/dbus/dbuslockfrontservice.cpp"
    "${LOCK_CLASSIC_DIR}/lock/freedesktopscreensaver.cpp"
    "${LOCK_CLASSIC_DIR}/lock/waylandsessionlock.cpp"
    "${LOCK_REPO_ROOT}/src/common/LockAppearance.cpp"
    "${LOCK_REPO_ROOT}/src/common/FingerprintProbe.cpp"
    "${LOCK_REPO_ROOT}/src/common/FingerprintSession.cpp"
    "${LOCK_REPO_ROOT}/src/common/LockState.cpp"
    "${LOCK_REPO_ROOT}/src/common/LockTranslations.cpp"
    "${LOCK_REPO_ROOT}/src/common/LogindSession.cpp"
    ${LOCK_VENDOR_SOURCES}
    ${LOCK_AUTH_FILES}
)

set(LOCK_BACKEND_HEADERS
    "${LOCK_CLASSIC_DIR}/session_widgets/authinterface.h"
    "${LOCK_CLASSIC_DIR}/session_widgets/sessionbasemodel.h"
    "${LOCK_CLASSIC_DIR}/session_widgets/userinfo.h"
    "${LOCK_CLASSIC_DIR}/lock/fingerprintunlock.h"
    "${LOCK_CLASSIC_DIR}/lock/lockworker.h"
    "${LOCK_CLASSIC_DIR}/lock/lockshortcutmanager.h"
    "${LOCK_CLASSIC_DIR}/lock/dbus/dbuslockagent.h"
    "${LOCK_CLASSIC_DIR}/lock/dbus/dbuslockfrontservice.h"
    "${LOCK_CLASSIC_DIR}/lock/freedesktopscreensaver.h"
    "${LOCK_CLASSIC_DIR}/lock/waylandsessionlock.h"
    "${LOCK_REPO_ROOT}/src/common/LockAppearance.h"
    "${LOCK_REPO_ROOT}/src/common/FingerprintProbe.h"
    "${LOCK_REPO_ROOT}/src/common/FingerprintSession.h"
    "${LOCK_REPO_ROOT}/src/common/LockState.h"
    "${LOCK_REPO_ROOT}/src/common/LockTranslations.h"
    "${LOCK_REPO_ROOT}/src/common/LogindSession.h"
    ${LOCK_VENDOR_HEADERS}
)

add_library(gxde-lock-backend STATIC
    ${LOCK_BACKEND_SOURCES}
    ${LOCK_BACKEND_HEADERS}
)

target_include_directories(gxde-lock-backend PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${LOCK_CLASSIC_DIR}"
    "${LOCK_CLASSIC_DIR}/global_util"
    "${LOCK_CLASSIC_DIR}/global_util/dbus"
    "${LOCK_CLASSIC_DIR}/session_widgets"
    "${LOCK_CLASSIC_DIR}/lock"
    "${LOCK_CLASSIC_DIR}/lock/dbus"
    "${LOCK_CLASSIC_DIR}/vendor"
    "${LOCK_REPO_ROOT}/src/auth"
    "${LOCK_REPO_ROOT}/src/common"
    "${LOCK_CONSTANTS_INCLUDE_DIR}"
    "${LOCK_CLASSIC_BINARY_DIR}"
)

target_link_libraries(gxde-lock-backend PUBLIC
    ${NEO_COMMON_LIBS}
    gxde-lock-ext-session-lock-protocol
    Qt6::Qml
    Qt6::WaylandClientPrivate
)

# QML-safe facade, user carousel and style resolution.
add_library(gxde-lock-common STATIC
    lockbackend.cpp
    lockbackend.h
    userlistmodel.cpp
    userlistmodel.h
    lockstyle.cpp
    lockstyle.h
    lockstylemanager.cpp
    lockstylemanager.h
    mediacontroller.cpp
    mediacontroller.h
    notificationcounter.cpp
    notificationcounter.h
    systemvolume.cpp
    systemvolume.h
)

target_include_directories(gxde-lock-common PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_link_libraries(gxde-lock-common PUBLIC
    gxde-lock-backend
    Qt6::Core Qt6::Gui Qt6::DBus Qt6::Network
)

if(LOCK_BUILD_TESTS)
    add_subdirectory(tests)
endif()
