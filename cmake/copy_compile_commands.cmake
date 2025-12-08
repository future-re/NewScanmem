if (EXISTS "${CMAKE_BINARY_DIR}/compile_commands.json")
    file(COPY "${CMAKE_BINARY_DIR}/compile_commands.json"
         DESTINATION "${CMAKE_SOURCE_DIR}/"
         FILE_PERMISSIONS OWNER_READ OWNER_WRITE)
    message(STATUS "Copied compile_commands.json from build to project root for clangd")
else()
    message(STATUS "No compile_commands.json found in build directory; skipping copy")
endif()
