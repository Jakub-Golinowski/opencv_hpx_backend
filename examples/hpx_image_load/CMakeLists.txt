# Require a recent version of cmake
cmake_minimum_required(VERSION 3.3.2 FATAL_ERROR)

# This project is C++ based.
project(hpx_image_load CXX)
#set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

# Instruct cmake to find the HPX settings
find_package(HPX)
find_package( OpenCV REQUIRED )
include_directories( ${OpenCV_INCLUDE_DIRS} )

set(hpx_image_load_FLAGS DEPENDENCIES iostreams_component)

# Build your application using HPX
add_hpx_executable(hpx_image_load
        ESSENTIAL
        SOURCES hpx_image_load.cpp system_characteristics.hpp
        #DEPENDENCIES iostreams_component
        COMPONENT_DEPENDENCIES iostreams)

target_link_libraries(hpx_image_load_exe ${OpenCV_LIBS} )

if(MSVC)
    add_definitions(-D_USE_MATH_DEFINES)
endif()