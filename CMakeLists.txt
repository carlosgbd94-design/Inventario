cmake_minimum_required(VERSION 3.21)
project(Inventario VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

list(APPEND CMAKE_PREFIX_PATH "C:/Qt/6.10.3/msvc2022_64")

find_package(Qt6 REQUIRED COMPONENTS Widgets Sql Network PrintSupport Svg SvgWidgets)
qt_standard_project_setup()

configure_file(
    "${CMAKE_SOURCE_DIR}/src/version.h.in"
    "${CMAKE_BINARY_DIR}/generated/version.h"
    @ONLY
)

qt_add_executable(Inventario WIN32
    src/main.cpp
    src/app/ThemeManager.h
    src/app/ThemeManager.cpp
    src/ui/MainWindow.h
    src/ui/MainWindow.cpp
    resources/app.qrc
)

target_include_directories(Inventario PRIVATE
    "${CMAKE_SOURCE_DIR}/src"
    "${CMAKE_BINARY_DIR}/generated"
)

target_link_libraries(Inventario PRIVATE
    Qt6::Widgets
    Qt6::Sql
    Qt6::Network
    Qt6::PrintSupport
    Qt6::Svg
    Qt6::SvgWidgets
)

set_target_properties(Inventario PROPERTIES
    OUTPUT_NAME "Inventario"
)

if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()
