# Require a recent version of cmake
cmake_minimum_required(VERSION 3.3.2 FATAL_ERROR)

# This project is C++ based.
project(hpx_opencv_webcam CXX)

# Instruct cmake to find the HPX settings
find_package(HPX REQUIRED)
find_package(OpenCV REQUIRED)

#TODO add OpenCV_LIBS to hpx_executable DEPENDENCIES
# Build your application using HPX
add_hpx_executable(hpx_opencv_webcam
        ESSENTIAL
        SOURCES hpx_opencv_webcam.cpp system_characteristics.hpp
        COMPONENT_DEPENDENCIES iostreams)

target_link_libraries(hpx_opencv_webcam_exe ${OpenCV_LIBS})

if(MSVC)
    add_definitions(-D_USE_MATH_DEFINES)
endif()

get_directory_property(hasParent PARENT_DIRECTORY)
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
# so that we will find TutorialConfig.h
include_directories("${PROJECT_BINARY_DIR}")
