include(CheckLibraryExists)

# gxde-lock-auth-helper: runs the gxde-lock PAM stack as the locked user and
# reports the result to the locker over a local socket (SDDM::Auth protocol).
set(HELPER_SOURCES
    ${CMAKE_SOURCE_DIR}/src/common/SafeDataStream.cpp
    ${CMAKE_SOURCE_DIR}/src/auth/Auth.cpp
    ${CMAKE_SOURCE_DIR}/src/auth/AuthRequest.cpp
    ${CMAKE_SOURCE_DIR}/src/auth/AuthPrompt.cpp
    Backend.cpp
    HelperApp.cpp
    backend/PamHandle.cpp
    backend/PamBackend.cpp
)

add_executable(gxde-lock-auth-helper ${HELPER_SOURCES})
target_include_directories(gxde-lock-auth-helper PRIVATE
    "${CMAKE_SOURCE_DIR}/src/common"
    "${CMAKE_SOURCE_DIR}/src/auth"
    "${CMAKE_BINARY_DIR}/src/common"
    ${PAM_INCLUDE_DIR}
)
target_link_libraries(gxde-lock-auth-helper PRIVATE
    Qt6::Network
    Qt6::Qml
    ${PAM_LIBRARIES}
)

if(JOURNALD_FOUND)
    target_link_libraries(gxde-lock-auth-helper PRIVATE ${JOURNALD_LIBRARIES})
endif()

install(TARGETS gxde-lock-auth-helper RUNTIME DESTINATION "${CMAKE_INSTALL_LIBEXECDIR}")
