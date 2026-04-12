cmake_minimum_required(VERSION 3.10)

project(IdleProfilePlugin)

set(CMAKE_AUTOMOC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Widgets)

add_library(IdleProfilePlugin SHARED
    IdleProfilePlugin.cpp
    IdleProfileWidget.cpp
)

target_compile_features(IdleProfilePlugin PRIVATE cxx_std_17)

set(OPENRGB_SOURCE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../..")

target_include_directories(IdleProfilePlugin PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${OPENRGB_SOURCE_ROOT}
    ${OPENRGB_SOURCE_ROOT}/i2c_smbus
)

target_link_libraries(IdleProfilePlugin
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets
)