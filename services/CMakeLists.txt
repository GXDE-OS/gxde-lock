if(INSTALL_PAM_CONFIGURATION)
    set(PAM_DIR "${CMAKE_INSTALL_FULL_SYSCONFDIR}/pam.d")
    if(EXISTS "/etc/debian_version")
        install(FILES debian.gxde-lock.pam DESTINATION ${PAM_DIR} RENAME gxde-lock)
        install(FILES debian.gxde-lock-fingerprint.pam DESTINATION ${PAM_DIR} RENAME gxde-lock-fingerprint)
    elseif(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
        install(FILES freebsd.gxde-lock.pam DESTINATION ${PAM_DIR} RENAME gxde-lock)
        # No gxde-lock-fingerprint: fprintd is not part of the FreeBSD base.
        # Fingerprint unlock only starts when fprintd reports a reader.
    else()
        install(FILES gxde-lock.pam DESTINATION ${PAM_DIR} RENAME gxde-lock)
        install(FILES gxde-lock-fingerprint.pam DESTINATION ${PAM_DIR} RENAME gxde-lock-fingerprint)
    endif()
endif()
