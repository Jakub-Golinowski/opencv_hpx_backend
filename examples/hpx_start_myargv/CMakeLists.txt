# Require a recent version of cmake
cmake_minimum_required(VERSION 3.5.1 FATAL_ERROR)

# This project is C++ based.
project(hpx_start_myargv CXX)

# Instruct cmake to find the HPX settings
find_package(HPX REQUIRED)

#build your application using HPX
add_hpx_executable(hpx_start_myargv
        ESSENTIAL
        SOURCES hpx_start_myargv.cpp
        COMPONENT_DEPENDENCIES iostreams)
