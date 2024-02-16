cmake_minimum_required(VERSION 3.9)

project(tuilight VERSION 0.0.1 DESCRIPTION "A light TUI library")

add_library(${PROJECT_NAME} SHARED
    src/element.cpp
    src/terminal.cpp
    src/view.cpp
)
set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${PROJECT_VERSION})
target_include_directories(${PROJECT_NAME} PUBLIC src)
