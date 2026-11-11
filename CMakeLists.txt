cmake_minimum_required(VERSION 3.20)
project(gxde-lock VERSION "0.3.2" LANGUAGES CXX C)

set(UNRELEASED_FEATURES)

set(QT_MIN_VERSION "6.6.0")
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(QS_BUILD_OPTIONS "")

set(CRASHREPORT_URL "" CACHE STRING "Bugreport URL")

function(boption VAR NAME DEFAULT)
	cmake_parse_arguments(PARSE_ARGV 3 arg "" "REQUIRES" "")

	option(${VAR} ${NAME} ${DEFAULT})

	set(STATUS "${VAR}_status")
	set(EFFECTIVE "${VAR}_effective")
	set(${STATUS} ${${VAR}})
	set(${EFFECTIVE} ${${VAR}})

	if (${${VAR}} AND DEFINED arg_REQUIRES)
		set(REQUIRED_EFFECTIVE "${arg_REQUIRES}_effective")
		if (NOT ${${REQUIRED_EFFECTIVE}})
			set(${STATUS} "OFF (Requires ${arg_REQUIRES})")
			set(${EFFECTIVE} OFF)
		endif()
	endif()

	set(${EFFECTIVE} "${${EFFECTIVE}}" PARENT_SCOPE)

	message(STATUS "  ${NAME}: ${${STATUS}}")

	string(APPEND QS_BUILD_OPTIONS "\\n  ${NAME}: ${${STATUS}}")
	set(QS_BUILD_OPTIONS "${QS_BUILD_OPTIONS}" PARENT_SCOPE)
endfunction()

set(DISTRIBUTOR "Unset" CACHE STRING "Distributor")
string(APPEND QS_BUILD_OPTIONS "  Distributor: ${DISTRIBUTOR}")

message(STATUS "gxde-lock configuration")
message(STATUS "  Distributor: ${DISTRIBUTOR}")
boption(NO_PCH "Disable precompild headers (dev)" OFF)
boption(BUILD_TESTING "Build tests (dev)" OFF)
boption(DEV_QML "Load lock screen QML from the source tree (dev)" OFF)
boption(ASAN "ASAN (dev)" OFF) # note: better output with gcc than clang
boption(FRAME_POINTERS "Keep Frame Pointers (dev)" ${ASAN})

if (CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
	boption(USE_JEMALLOC "Use jemalloc" OFF)
else()
	boption(USE_JEMALLOC "Use jemalloc" ON)
endif()
boption(CRASH_HANDLER "Crash Handling" ON)
boption(SOCKETS "Unix Sockets" ON)
boption(WAYLAND "Wayland" ON)
boption(WAYLAND_SESSION_LOCK "  Session Lock" ON REQUIRES WAYLAND)
boption(X11 "X11" ON)
boption(SERVICE_PAM "Pam" ON)

include(cmake/install-qml-module.cmake)
include(cmake/util.cmake)

add_compile_options(-Wall -Wextra -Wno-vla-cxx-extension)

add_compile_options(-fno-strict-overflow)

if (FRAME_POINTERS)
	add_compile_options(-fno-omit-frame-pointer)
endif()

if (ASAN)
	add_compile_options(-fsanitize=address)
	add_link_options(-fsanitize=address)
endif()

# nix workaround
if (CMAKE_EXPORT_COMPILE_COMMANDS)
	set(CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES})
endif()

if (NOT CMAKE_BUILD_TYPE)
	message(STATUS "CMAKE_BUILD_TYPE unset, defaulting to Debug.")
	set(CMAKE_BUILD_TYPE Debug)
endif()

set(QT_FPDEPS Gui Qml Quick QuickControls2 Widgets ShaderTools)
set(QT_PRIVDEPS QuickPrivate)

include(cmake/pch.cmake)

if (BUILD_TESTING)
	enable_testing()
	add_definitions(-DQS_TEST)
	list(APPEND QT_FPDEPS Test)
endif()

if (SOCKETS)
	list(APPEND QT_FPDEPS Network)
endif()

if (WAYLAND)
	list(APPEND QT_FPDEPS WaylandClient)
	list(APPEND QT_PRIVDEPS WaylandClientPrivate)
endif()

find_package(Qt6 REQUIRED COMPONENTS ${QT_FPDEPS})

# In Qt 6.10, private dependencies are required to be explicit,
# but they could not be explicitly depended on prior to 6.9.
if (Qt6_VERSION VERSION_GREATER_EQUAL "6.9.0")
	set(QT_NO_PRIVATE_MODULE_WARNING ON)
	find_package(Qt6 REQUIRED COMPONENTS ${QT_PRIVDEPS})
endif()

set(CMAKE_AUTOUIC OFF)
qt_standard_project_setup(REQUIRES 6.6)
set(QT_QML_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/qml_modules)

add_subdirectory(src)

install(DIRECTORY qml/ DESTINATION ${CMAKE_INSTALL_DATADIR}/gxde-lock/qml)

if (USE_JEMALLOC)
	find_package(PkgConfig REQUIRED)
	# IMPORTED_TARGET not working for some reason
	pkg_check_modules(JEMALLOC REQUIRED jemalloc)
	target_link_libraries(quickshell PRIVATE ${JEMALLOC_LIBRARIES})
endif()
