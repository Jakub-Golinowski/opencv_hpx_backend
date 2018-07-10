# Require a recent version of cmake
cmake_minimum_required(VERSION 3.3.2 FATAL_ERROR)

# This project is C++ based.
project(hpx_start_stop CXX)
find_package(OpenCV REQUIRED)

# Instruct cmake to find the HPX settings
find_package(HPX)

#build your application using HPX
add_hpx_executable(start_stop
        ESSENTIAL
        SOURCES start_stop.cpp
        COMPONENT_DEPENDENCIES iostreams)

target_include_directories(start_stop_exe PRIVATE ${OpenCV_INCLUDE_DIRS})
