cmake_minimum_required(VERSION 3.12)

project(tuilight VERSION 0.0.1 DESCRIPTION "A light TUI library")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_library(${PROJECT_NAME} SHARED
src/element.cpp
src/terminal.cpp
src/view.cpp
)

set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${PROJECT_VERSION})
target_include_directories(${PROJECT_NAME} PUBLIC src)

option(BUILD_DEV "Build the development testing executable" OFF)
if(BUILD_DEV)
    add_executable(main src/main.cpp)
    target_link_libraries(main ${PROJECT_NAME})
endif()

find_program(CLANG_TIDY_BIN NAMES "clang-tidy")
set(CLANG_TIDY_COMMAND ${CLANG_TIDY_BIN} --config-file=${CMAKE_SOURCE_DIR}/.clang-tidy)
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_CLANG_TIDY "${CLANG_TIDY_COMMAND}")
