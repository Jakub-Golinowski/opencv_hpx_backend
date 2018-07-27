cmake_minimum_required(VERSION 3.5.1 FATAL_ERROR)

project(Example CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
#-----------------------------------------------------------------------------
# Find QT
#-----------------------------------------------------------------------------
find_package(Qt5 REQUIRED COMPONENTS Core Widgets Gui Svg OpenGL Concurrent)

# set(project_ui
#     mainwindow.ui)

set(project_headers
        widget.hpp
        capturethread.hpp
        ConcurrentCircularBuffer.hpp
        renderwidget.hpp
        filter.cpp)

set(project_sources
        main.cpp
        widget.cpp
        capturethread.cpp
        renderwidget.cpp
        filter.cpp)

# qt5_wrap_ui(project_headers_wrapped ${project_ui})
qt5_wrap_cpp(project_sources_moc ${project_headers} OPTIONS -DBOOST_TT_HAS_OPERATOR_HPP_INCLUDED)
#-----------------------------------------------------------------------------
# Find Boost
#-----------------------------------------------------------------------------
#option(Boost_USE_STATIC "Use static Boost libraries" ON)
find_package(Boost 1.49 REQUIRED thread system)
link_directories(${Boost_LIBRARY_DIRS})
include_directories(${Boost_INCLUDE_DIR})
message("Boost library dir is ${Boost_LIBRARY_DIRS}")
message("Boost include dir is ${Boost_INCLUDE_DIRS}")

#-----------------------------------------------------------------------------
# Find HPX
#-----------------------------------------------------------------------------
find_package(HPX REQUIRED)
message("HPX libs are " ${HPX_LIBRARIES})

#-----------------------------------------------------------------------------
# Find OpenCV
#-----------------------------------------------------------------------------
find_package(OpenCV REQUIRED)
message("opencv libs are " ${OpenCV_LIBS})


add_hpx_executable(qt
                   MODULE qt
                   DEPENDENCIES ${OpenCV_LIBS} ${Boost_LIBRARIES}
                   SOURCES ${project_headers} ${project_sources} ${project_sources_moc}
                   COMPONENT_DEPENDENCIES iostreams
)

target_link_libraries(qt_exe
                      ${HPX_TLL_PUBLIC}
                      Qt5::Core
                      Qt5::Widgets
                      Qt5::Gui
                      Qt5::Svg
                      Qt5::OpenGL
                      Qt5::Concurrent
  )
