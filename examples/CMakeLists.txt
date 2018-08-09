set(subdirs
    hpx_mandelbrot
    hpx_start_myargv
    hpx_start_stop
    opencv_mandelbrot
    qt_with_cmake
 )

if(BACKEND_STARTSTOP)
    message("WARN: BACKEND_STARTSTOP=${BACKEND_STARTSTOP} because of lack of compatibility with start-stop backend not building:")
    message("      - hpx_image_load")
    message("      - hpx_opencv_webcam")
else()
    set(subdirs ${subdirs}
    hpx_image_load
    hpx_opencv_webcam
    qt_hpx_opencv
    )
endif()


foreach(subdir ${subdirs})
  add_subdirectory(${subdir})
endforeach()