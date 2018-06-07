# Require a recent version of cmake
cmake_minimum_required(VERSION 3.3.2 FATAL_ERROR)

# This project is C++ based.
project(opencv_mandelbrot CXX)

# Instruct cmake to find the HPX settings
find_package(HPX REQUIRED)
find_package(OpenCV REQUIRED)

# Build your application using HPX
add_hpx_executable(opencv_mandelbrot
        ESSENTIAL
        SOURCES opencv_mandelbrot.cpp system_characteristics.hpp
        COMPONENT_DEPENDENCIES iostreams)

target_include_directories(opencv_mandelbrot_exe PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(opencv_mandelbrot_exe ${OpenCV_LIBS} )



option(BACKEND_STARTSTOP "Use backend that is starting and stopping the HPX runtime on its own" OFF)
if(BACKEND_STARTSTOP)
  set(BACKEND_STARTSTOP)
endif(BACKEND_STARTSTOP)



configure_file(config.hpp.in config.hpp)
include_directories(${CMAKE_CURRENT_BINARY_DIR})


if(MSVC)
    add_definitions(-D_USE_MATH_DEFINES)
endif()