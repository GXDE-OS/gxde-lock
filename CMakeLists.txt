cmake_minimum_required(VERSION 3.16)

project(gxde-lock LANGUAGES C CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# Options
option(ENABLE_JOURNALD "Enable logging to journald" ON)
option(NO_SYSTEMD "Disable systemd support" OFF)
option(USE_ELOGIND "Use elogind instead of logind" OFF)
option(BUILD_NEO_LOCK "Build the QML lock screen (gxde-lock-neo)" ON)
option(LOCK_BUILD_TESTS "Build lock screen backend tests" OFF)
option(INSTALL_PAM_CONFIGURATION "Install PAM configuration files" ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake;${CMAKE_MODULE_PATH}")

# Definitions
add_definitions(-Wall -Wextra -DQT_NO_CAST_FROM_ASCII -DQT_NO_CAST_FROM_BYTEARRAY -DQT_NO_FOREACH)

# Default build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Handle build type
if(CMAKE_BUILD_TYPE MATCHES [Dd]ebug)
    message(STATUS "Debug build")
    add_definitions(-DDEBUG)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0")
else()
    message(STATUS "Release build")
    add_definitions(-DNDEBUG)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2")
endif()

# Default absolute paths
if(NOT DEFINED CMAKE_INSTALL_SYSCONFDIR)
    set(CMAKE_INSTALL_SYSCONFDIR "/etc")
endif()
if(NOT DEFINED CMAKE_INSTALL_LOCALSTATEDIR)
    set(CMAKE_INSTALL_LOCALSTATEDIR "/var")
endif()

# Standard installation paths
include(GNUInstallDirs)

# Macros
include(FeatureSummary)

# PKG-CONFIG
find_package(PkgConfig)

# PAM
find_package(PAM REQUIRED)

# Qt
find_package(Qt6 6.5 CONFIG REQUIRED Core DBus Gui Qml Quick Network Test)

# systemd
if(NOT NO_SYSTEMD AND NOT USE_ELOGIND)
    pkg_check_modules(SYSTEMD "systemd")
endif()

if(SYSTEMD_FOUND)
    add_definitions(-DHAVE_SYSTEMD)
    set(CMAKE_AUTOMOC_MOC_OPTIONS -DHAVE_SYSTEMD)

    # libsystemd-journal was merged into libsystemd in 209
    if(${SYSTEMD_VERSION} VERSION_LESS 209)
        pkg_check_modules(JOURNALD "libsystemd-journal")
    else()
        pkg_check_modules(JOURNALD "libsystemd")
    endif()

    if(ENABLE_JOURNALD)
        if(JOURNALD_FOUND)
            add_definitions(-DHAVE_JOURNALD)
            set(CMAKE_AUTOMOC_MOC_OPTIONS -DHAVE_JOURNALD)
        else()
            message(WARNING "Disable journald support for lack of libsystemd-journal")
        endif()
    endif()

    set(HALT_COMMAND "/usr/bin/systemctl poweroff")
    set(REBOOT_COMMAND "/usr/bin/systemctl reboot")
else()
    set(SYSTEMD_FOUND 0)
endif()
add_feature_info("systemd" SYSTEMD_FOUND "systemd support")
add_feature_info("journald" JOURNALD_FOUND "journald support")

# elogind
if(NO_SYSTEMD AND USE_ELOGIND)
    pkg_check_modules(ELOGIND "libelogind")
endif()

if(ELOGIND_FOUND)
    add_definitions(-DHAVE_ELOGIND)
    set(CMAKE_AUTOMOC_MOC_OPTIONS -DHAVE_ELOGIND)

    set(HALT_COMMAND "/usr/bin/loginctl poweroff")
    set(REBOOT_COMMAND "/usr/bin/loginctl reboot")
endif()
add_feature_info("elogind" ELOGIND_FOUND "elogind support")

# Default behaviour if neither systemd nor elogind is used
if (NOT ELOGIND_FOUND AND NOT SYSTEMD_FOUND)
    # Shutdown and reboot commands for the root helper's power actions.
    if("${CMAKE_SYSTEM_NAME}" STREQUAL "FreeBSD")
        set(HALT_COMMAND "/sbin/shutdown -p now")
    else()
        set(HALT_COMMAND "/sbin/shutdown -h -P now")
    endif()
    set(REBOOT_COMMAND "/sbin/shutdown -r now")
endif()

# Set constants
set(STATE_DIR                   "${CMAKE_INSTALL_FULL_LOCALSTATEDIR}/lib/gxde-lock" CACHE PATH      "State directory of the root helper")
set(CONFIG_FILE                 "${CMAKE_INSTALL_FULL_SYSCONFDIR}/gxde-lock.conf"   CACHE PATH      "Path of the gxde-lock config file")
set(OMG_CONFIG_FILE             "${CMAKE_INSTALL_FULL_SYSCONFDIR}/gxde-lock/OMG.conf" CACHE PATH    "Path of the Oh-My-GXDE config file")
set(DBUS_SYSTEM_CONFIG_DIR      "${CMAKE_INSTALL_FULL_DATADIR}/dbus-1/system.d"     CACHE PATH      "D-Bus system bus policy directory")

# Add subdirectories
enable_testing()
configure_file("src/common/Constants.h.in" "src/common/Constants.h" IMMEDIATE @ONLY)
add_subdirectory(data)
add_subdirectory(services)
add_subdirectory(src)
add_subdirectory(translations)

add_subdirectory(test)

# Display feature summary
feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
