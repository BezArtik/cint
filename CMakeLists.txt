cmake_minimum_required(VERSION 3.20...4.1)
project(interpreter LANGUAGES CXX)

set(CMAKE_CXX_SCAN_FOR_MODULES OFF)
add_custom_target(compile_commands_symlink ALL
  COMMAND ${CMAKE_COMMAND} -E create_symlink
    ${CMAKE_BINARY_DIR}/compile_commands.json
    ${CMAKE_SOURCE_DIR}/compile_commands.json
  COMMENT "Symlink compile_commands.json to project root"
)

option(BUILD_TESTS "Build tests" ON)
option(ENABLE_SANITIZERS "Enable ASan and UBSan" OFF)
option(ENABLE_WARNINGS_AS_ERRORS "Treat warnings as errors" OFF)
option(BUILD_DOCS "Build Documentation" OFF)

add_subdirectory(src)

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(BUILD_DOCS)
    find_package(Doxygen QUIET)
    if(DOXYGEN_FOUND)
        set(DOXYFILE_PATH "${CMAKE_SOURCE_DIR}/Doxyfile")
        
        if(NOT EXISTS "${DOXYFILE_PATH}")
            message(FATAL_ERROR "Doxyfile not found at: ${DOXYFILE_PATH}")
        endif()
        
        add_custom_target(doc
            COMMAND ${DOXYGEN_EXECUTABLE} "${DOXYFILE_PATH}"
            WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}" 
            COMMENT "Generating API documentation with Doxygen"
            VERBATIM
        )
        
        add_custom_target(open-doc
            COMMAND xdg-open "${CMAKE_SOURCE_DIR}/html/index.html"
            COMMENT "Opening documentation in browser"
            VERBATIM
        )
        
        message(STATUS "Doxygen target 'doc' created")
        message(STATUS "  Doxyfile: ${DOXYFILE_PATH}")
        message(STATUS "  Output:   ${CMAKE_SOURCE_DIR}/html/")
    else()
        message(WARNING "Doxygen not found! Install with: sudo apt install doxygen")
    endif()
endif()
