cmake_minimum_required(VERSION 3.10)

project(IdleProfilePlugin)

set(CMAKE_AUTOMOC ON)

set(OPENRGB_SOURCE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.." CACHE PATH "Path to the OpenRGB source tree")

if(NOT EXISTS "${OPENRGB_SOURCE_ROOT}/OpenRGB/OpenRGBPluginInterface.h")
    message(FATAL_ERROR "OPENRGB_SOURCE_ROOT must point to an OpenRGB source tree containing OpenRGB/OpenRGBPluginInterface.h")
endif()

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Widgets)

add_library(IdleProfilePlugin SHARED
    IdleProfilePlugin.cpp
    IdleProfileWidget.cpp
)

target_compile_features(IdleProfilePlugin PRIVATE cxx_std_17)

target_include_directories(IdleProfilePlugin PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${OPENRGB_SOURCE_ROOT}
    ${OPENRGB_SOURCE_ROOT}/OpenRGB
    ${OPENRGB_SOURCE_ROOT}/OpenRGB/i2c_smbus
)

target_link_libraries(IdleProfilePlugin
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets
)