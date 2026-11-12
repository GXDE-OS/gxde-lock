# One catalog, gxde-lock_<language>.ts, for every user-visible string of the
# lock screens and gxde-lock-guard. English is the source language;
# gxde-lock_en.ts only supplies English plural forms.
#
# After changing strings, refresh the catalogs with
#   cmake --build build-cmake --target gxde-lock-lupdate
# then translate the new entries (Qt Linguist or a text editor).

find_package(Qt6 REQUIRED COMPONENTS LinguistTools)

set(GXDE_LOCK_LANGUAGES ar en es fr ru zh_CN)

set(GXDE_LOCK_TS_FILES)
set(GXDE_LOCK_QM_FILES)
foreach(lang ${GXDE_LOCK_LANGUAGES})
    set(ts "${CMAKE_CURRENT_SOURCE_DIR}/gxde-lock_${lang}.ts")
    set(qm "${CMAKE_CURRENT_BINARY_DIR}/gxde-lock_${lang}.qm")
    list(APPEND GXDE_LOCK_TS_FILES "${ts}")
    list(APPEND GXDE_LOCK_QM_FILES "${qm}")
    add_custom_command(
        OUTPUT "${qm}"
        COMMAND Qt6::lrelease -silent "${ts}" -qm "${qm}"
        DEPENDS "${ts}"
        VERBATIM
    )
endforeach()

add_custom_target(gxde-lock-translations ALL DEPENDS ${GXDE_LOCK_QM_FILES})
install(FILES ${GXDE_LOCK_QM_FILES}
        DESTINATION "${CMAKE_INSTALL_DATADIR}/gxde-lock/translations")

add_custom_target(gxde-lock-lupdate
    COMMAND Qt6::lupdate -recursive -no-obsolete -locations none
            -extensions cpp,cc,h,qml
            src/common src/guard src/lock-classic src/lock-common src/lock-neo
            -ts ${GXDE_LOCK_TS_FILES}
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    VERBATIM
)
