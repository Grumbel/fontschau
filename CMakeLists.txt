cmake_minimum_required(VERSION 3.16)
project(Fontora VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_executable(fontora
    src/main.cpp
    src/mainwindow.cpp
    src/mainwindow.h
    src/fontpreview.cpp
    src/fontpreview.h
)

target_link_libraries(fontora PRIVATE Qt6::Widgets)

install(TARGETS fontora DESTINATION bin)