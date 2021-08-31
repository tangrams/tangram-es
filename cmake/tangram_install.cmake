function(tangram_install TARGET)
    install(TARGETS ${TARGET}
        EXPORT tangramTargets
        LIBRARY DESTINATION ${TANGRAM_LIB_DIR}
        ARCHIVE DESTINATION ${TANGRAM_ARCHIVE_DIR}
        RUNTIME DESTINATION ${TANGRAM_BIN_DIR}
        INCLUDES DESTINATION include/
        PUBLIC_HEADER DESTINATION include/
        COMPONENT tangram
    )
endfunction()

function(tangram_install_dir DIR)
    install(DIRECTORY ${DIR} TYPE INCLUDE)
endfunction()

function(tangram_install_files INSTALL_FILES)
    install(FILES ${INSTALL_FILES} TYPE INCLUDE)
endfunction()
