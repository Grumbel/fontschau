# SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
# SPDX-License-Identifier: GPL-3.0-or-later

cmake_minimum_required(VERSION 3.16)

# VERSION is the single source of truth. Packaging may provide the full
# development revision in PROJECT_VERSION_FULL (e.g. via a Nix flake).
# Reconfigure when the base version changes (not just when CMakeLists.txt does).
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/VERSION")
if(NOT DEFINED PROJECT_VERSION_FULL)
    file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" PROJECT_VERSION_FULL LIMIT_COUNT 1)
    if(PROJECT_VERSION_FULL MATCHES "-dev")
        find_package(Git QUIET)
        set(_revision_count 0)
        set(_short_rev "dirty")
        if(GIT_FOUND AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.git")
            # Refresh the development build's revision after a checkout/commit.
            set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
                "${CMAKE_CURRENT_SOURCE_DIR}/.git/HEAD"
                "${CMAKE_CURRENT_SOURCE_DIR}/.git/index"
                "${CMAKE_CURRENT_SOURCE_DIR}/.git/logs/HEAD")
            execute_process(
                COMMAND "${GIT_EXECUTABLE}" rev-list --count HEAD
                WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
                OUTPUT_VARIABLE _git_count OUTPUT_STRIP_TRAILING_WHITESPACE
                ERROR_QUIET RESULT_VARIABLE _git_count_result
            )
            execute_process(
                COMMAND "${GIT_EXECUTABLE}" rev-parse --short HEAD
                WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
                OUTPUT_VARIABLE _git_hash OUTPUT_STRIP_TRAILING_WHITESPACE
                ERROR_QUIET RESULT_VARIABLE _git_hash_result
            )
            execute_process(
                COMMAND "${GIT_EXECUTABLE}" status --porcelain --untracked-files=normal
                WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
                OUTPUT_VARIABLE _git_dirty OUTPUT_STRIP_TRAILING_WHITESPACE
                ERROR_QUIET RESULT_VARIABLE _git_dirty_result
            )
            if(_git_count_result EQUAL 0 AND _git_count MATCHES "^[0-9]+$")
                set(_revision_count "${_git_count}")
            endif()
            if(_git_hash_result EQUAL 0 AND _git_hash MATCHES "^[a-f0-9]+$")
                set(_short_rev "${_git_hash}")
                if(NOT _git_dirty_result EQUAL 0 OR NOT _git_dirty STREQUAL "")
                    string(APPEND _short_rev "-dirty")
                endif()
            endif()
        endif()
        string(APPEND PROJECT_VERSION_FULL ".${_revision_count}+g${_short_rev}")
    endif()
endif()

string(REGEX MATCH "^[0-9]+(\\.[0-9]+)*" _project_version_cmake "${PROJECT_VERSION_FULL}")
if(NOT _project_version_cmake)
    message(FATAL_ERROR "Invalid project version: ${PROJECT_VERSION_FULL}")
endif()
project(Fontschau VERSION "${_project_version_cmake}" LANGUAGES CXX)
message(STATUS "Fontschau version: ${PROJECT_VERSION_FULL}")

configure_file(src/version.h.in "${CMAKE_CURRENT_BINARY_DIR}/generated/version.h" @ONLY)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Svg)

add_executable(fontschau
    src/main.cpp
    src/mainwindow.cpp
    src/mainwindow.h
    src/fontpreview.cpp
    src/fontpreview.h
    src/resources.qrc
)

target_link_libraries(fontschau PRIVATE Qt6::Widgets Qt6::Svg)
target_include_directories(fontschau PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated")

include(GNUInstallDirs)

install(TARGETS fontschau
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(FILES data/applications/fontschau.desktop
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications
)

install(FILES data/icons/hicolor/scalable/apps/fontschau.svg
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/scalable/apps
)

include(CTest)
if(BUILD_TESTING)
    add_test(NAME cli COMMAND "${CMAKE_COMMAND}"
        "-DAPP=$<TARGET_FILE:fontschau>"
        "-DVERSION=${PROJECT_VERSION_FULL}"
        "-DINVALID_FONT=${CMAKE_CURRENT_SOURCE_DIR}/README.md"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/cli.cmake")
endif()
