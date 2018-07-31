cmake_minimum_required(VERSION 3.5.1 FATAL_ERROR)

project(Example CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
#-----------------------------------------------------------------------------
# Find QT
#-----------------------------------------------------------------------------
find_package(Qt5 REQUIRED COMPONENTS Core Widgets Gui Svg OpenGL Concurrent)

#-----------------------------------------------------------------------------
# Qt resource files
#-----------------------------------------------------------------------------
set(project_qt_rccs
        resources.qrc
)

#-----------------------------------------------------------------------------
# Qt GUI interface wrapping
#-----------------------------------------------------------------------------
 set(project_ui
        martycam.ui
        settings.ui)
#-----------------------------------------------------------------------------
# Qt QObjects for MOC
#-----------------------------------------------------------------------------
set(project_moc_headers
        martycam.hpp
        renderwidget.hpp
        settings.hpp
        widget.hpp
        processingthread.hpp
)

set(project_headers
        capturethread.hpp
        ConcurrentCircularBuffer.hpp
        filter.hpp
        martycam.hpp
        MotionFilter.hpp
        processingthread.hpp
        renderwidget.hpp
        settings.hpp
        widget.hpp
)

set(project_sources
        capturethread.cpp
        filter.cpp
        main.cpp
        martycam.cpp
        MotionFilter.cpp
        processingthread.cpp
        renderwidget.cpp
        settings.cpp
        widget.cpp
)

qt5_wrap_ui(project_headers_wrapped ${project_ui})
qt5_wrap_cpp(project_sources_moc ${project_moc_headers} OPTIONS -DBOOST_TT_HAS_OPERATOR_HPP_INCLUDED)
QT5_ADD_RESOURCES(project_rcc_srcs ${project_qt_rccs})
#-----------------------------------------------------------------------------
# Find Boost
#-----------------------------------------------------------------------------
#option(Boost_USE_STATIC "Use static Boost libraries" ON)
find_package(Boost 1.58 REQUIRED thread system)
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
                   SOURCES ${project_headers} ${project_sources} ${project_sources_moc} ${project_headers_wrapped} ${project_rcc_srcs}
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
