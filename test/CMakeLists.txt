set(QT_USE_QTTEST TRUE)

find_package(Qt6 REQUIRED COMPONENTS Widgets)
pkg_check_modules(TEST_DTK2WIDGET REQUIRED IMPORTED_TARGET dtk2widget)

add_executable(SystemDefaultsTest
    SystemDefaultsTest.cpp
    ../src/common/SystemDefaults.cpp
    ../src/common/SystemDefaults.h
)
target_include_directories(SystemDefaultsTest PRIVATE
    ../src/common
)
add_test(NAME SystemDefaults COMMAND SystemDefaultsTest)
target_link_libraries(SystemDefaultsTest PRIVATE
    Qt6::Core
    Qt6::Test
)

add_executable(LockAppearanceTest
    LockAppearanceTest.cpp
    ../src/common/LockAppearance.cpp
    ../src/common/LockAppearance.h
)
target_include_directories(LockAppearanceTest PRIVATE
    ../src/common
    ${CMAKE_BINARY_DIR}/src/common
)
add_test(NAME LockAppearance COMMAND LockAppearanceTest)
target_link_libraries(LockAppearanceTest PRIVATE
    Qt6::Core
    Qt6::DBus
    Qt6::Gui
    Qt6::Test
)

add_executable(LockScaleTest
    LockScaleTest.cpp
    ../src/common/LockScale.cpp
    ../src/common/LockScale.h
)
target_include_directories(LockScaleTest PRIVATE
    ../src/common
)
add_test(NAME LockScale COMMAND LockScaleTest)
target_link_libraries(LockScaleTest PRIVATE
    Qt6::Core
    Qt6::Test
    X11
    Xrandr
)

add_executable(VirtualKeyboardTest
    VirtualKeyboardTest.cpp
    ../src/lock-classic/widgets/virtualkbinstance.cpp
    ../src/lock-classic/widgets/virtualkbinstance.h
)
add_test(NAME VirtualKeyboard COMMAND VirtualKeyboardTest -platform offscreen)
target_link_libraries(VirtualKeyboardTest PRIVATE
    Qt6::Widgets
    Qt6::Test
    PkgConfig::TEST_DTK2WIDGET
)

add_executable(MultiScreenManagerTest
    MultiScreenManagerTest.cpp
    ../src/lock-classic/global_util/multiscreenmanager.cpp
    ../src/lock-classic/global_util/multiscreenmanager.h
)
target_include_directories(MultiScreenManagerTest PRIVATE
    ../src/lock-classic/global_util
)
add_test(NAME MultiScreenManager
    COMMAND MultiScreenManagerTest -platform offscreen)
target_link_libraries(MultiScreenManagerTest PRIVATE
    Qt6::Widgets
    Qt6::Test
)

add_executable(GeneralSettingTest
    general_setting_test.cc
    ../src/system/general_setting.cc
)
target_include_directories(GeneralSettingTest PRIVATE ../src/system)
target_link_libraries(GeneralSettingTest PRIVATE Qt6::Core Qt6::Test)
add_test(NAME GeneralSetting COMMAND GeneralSettingTest)

# A private helper fixture exercises Auth's actual socket protocol without PAM.
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/auth-transition")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/auth-transition/Constants.h"
    "#define LIBEXEC_INSTALL_DIR \"${CMAKE_CURRENT_BINARY_DIR}/auth-transition\"\n")
add_executable(AuthTransitionTest
    AuthTransitionTest.cpp
    ../src/auth/Auth.cpp
    ../src/auth/AuthRequest.cpp
    ../src/auth/AuthPrompt.cpp
    ../src/common/SafeDataStream.cpp)
set_target_properties(AuthTransitionTest PROPERTIES
    OUTPUT_NAME gxde-lock-auth-helper
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/auth-transition")
target_include_directories(AuthTransitionTest BEFORE PRIVATE
    "${CMAKE_CURRENT_BINARY_DIR}/auth-transition" ../src/auth ../src/common)
target_link_libraries(AuthTransitionTest PRIVATE
    Qt6::Qml Qt6::Network Qt6::Test)
add_test(NAME AuthTransition COMMAND AuthTransitionTest)
set_tests_properties(AuthTransition PROPERTIES TIMEOUT 20)
