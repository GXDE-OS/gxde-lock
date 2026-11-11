# QML lock screen frontend. The authentication, user model, Wayland session
# lock and D-Bus surface come from gxde-lock-common / gxde-lock-backend; this
# target only owns the Qt Quick UI and the per-screen window lifecycle.

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Quick Qml)
qt_standard_project_setup(REQUIRES 6.5)

set(LOCK_NEO_QML_FILES
    qml/LockWindow.qml
    qml/LockScreen.qml
    qml/components/Clock.qml
    qml/components/LoginCard.qml
    qml/components/LockButton.qml
    qml/components/IconButton.qml
    qml/components/MediaIndicator.qml
    qml/components/NotificationPill.qml
    qml/components/ShadowText.qml
    qml/components/PowerMenu.qml
    qml/components/UserAvatar.qml
    qml/components/UserSwitcher.qml
    qml/components/VirtualKeyboardPanel.qml
)

# Flatten the QML module so LockWindow.qml can reference the components by
# their type names without a subdirectory import.
foreach(qml_file ${LOCK_NEO_QML_FILES})
    get_filename_component(qml_alias "${qml_file}" NAME)
    set_source_files_properties("${qml_file}" PROPERTIES
        QT_RESOURCE_ALIAS "${qml_alias}")
endforeach()

qt_add_resources(LOCK_NEO_RESOURCES
    res/resources.qrc
    lock-neo.qrc
)

qt_add_executable(gxde-lock-neo
    ${LOCK_NEO_RESOURCES}
    src/main.cc
    "${LOCK_REPO_ROOT}/src/common/LockCursor.cpp"
    "${LOCK_REPO_ROOT}/src/common/LockCursor.h"
    "${LOCK_REPO_ROOT}/src/common/LockScale.cpp"
    "${LOCK_REPO_ROOT}/src/common/LockScale.h"
)

qt_add_qml_module(gxde-lock-neo
    URI Lock
    VERSION 1.0
    QML_FILES ${LOCK_NEO_QML_FILES}
)

set_target_properties(gxde-lock-neo PROPERTIES
    OUTPUT_NAME gxde-lock-neo
    # Keep the build-tree binary next to gxde-lock-classic so the classic
    # locker's style handoff works without installing first.
    RUNTIME_OUTPUT_DIRECTORY "${LOCK_CLASSIC_BINARY_DIR}"
)

target_link_libraries(gxde-lock-neo PRIVATE
    gxde-lock-common
    Qt6::Quick
    Qt6::Qml
    LayerShellQt::Interface
    Qt6::WaylandClientPrivate
    X11
    Xcursor
    Xfixes
    Xrandr
)

install(TARGETS gxde-lock-neo
    BUNDLE DESTINATION .
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)
