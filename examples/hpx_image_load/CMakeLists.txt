# Require a recent version of cmake
cmake_minimum_required(VERSION 3.5.1 FATAL_ERROR)

# This project is C++ based.
project(hpx_image_load CXX)

# Instruct cmake to find the HPX settings
find_package(HPX REQUIRED)
find_package(OpenCV REQUIRED)

# Build your application using HPX
add_hpx_executable(hpx_image_load
        ESSENTIAL
        SOURCES hpx_image_load.cpp system_characteristics.hpp
        DEPENDENCIES ${OpenCV_LIBS}
        COMPONENT_DEPENDENCIES iostreams)

target_include_directories(hpx_image_load_exe PRIVATE ${OpenCV_INCLUDE_DIRS})

if(MSVC)
    add_definitions(-D_USE_MATH_DEFINES)
endif()

if(hasParent)
    message(STATUS "hpx_opencv_webcam Has a parent scope. DATA-PATH=${DATA_PATH}")
else()
    get_filename_component(DATA_PATH ../../data ABSOLUTE)
    message(STATUS "hpx_opencv_webcam Doesn't have a parent scope. DATA-PATH=${DATA_PATH}")
endif()


configure_file (
        "${PROJECT_SOURCE_DIR}/config.h.in"
        "${PROJECT_BINARY_DIR}/config.h"
)

# add the binary tree to the search path for include files
# so that we will find config.h
include_directories("${PROJECT_BINARY_DIR}")