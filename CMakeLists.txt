# SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
# SPDX-License-Identifier: GPL-3.0-or-later

cmake_minimum_required(VERSION 3.16)
project(Fontschau VERSION 0.1.0 LANGUAGES CXX)

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
