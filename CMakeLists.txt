cmake_minimum_required (VERSION 3.5.1)

project(HPX_backend_for_OpenCV CXX)

set(HPX_backend_for_OpenCV_MAJOR_VERSION 1)
set(HPX_backend_for_OpenCV_MINOR_VERSION 0)
set(HPX_backend_for_OpenCV_VERSION "${HPX_backend_for_OpenCV_MAJOR_VERSION}.${HPX_backend_for_OpenCV_MINOR_VERSION}")

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(DATA_PATH "${PROJECT_SOURCE_DIR}/data")

option(BACKEND_NON_HPX   "Set to ON if the OpenCV with non-HPX backend is used (e.g. pthreads or TBB)"   OFF)
option(BACKEND_STARTSTOP "Set to ON if the OpenCV with HPX backend in start/stop version is used"        OFF)

add_subdirectory(examples)