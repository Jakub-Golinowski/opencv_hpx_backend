# HPX backend for OpenCV

## Abstract

As a Google Summer of Code (GSoC) student in 2018 I developed an HPX backend for parallelism in OpenCV. Using OpenCV with the above mentioned HPX backend, while developing HPX-based application brings the following benefits:
1. the problem of competing backends is solved (more below).
1. user gets greater control over the execution of OpenCV parallel algorithms (more below).

#### Current state of the project
The backend was developed as part of the OpenCV code base and is now being reviewed as the Pull Request to OpenCV master branch ([current state of the PR to OpenCV][HPX_backend_PR])

## HPX Primer

HPX is a general purpose `C++` runtime system with a unified programming model transparently utilizing the available resources.
It does not use parallel-regions or fork-join approach, but launches an HPX runtime system with worker threads pinned to the processing units (PU).
Each of these worker threads is equipped with his own task scheduler, which introduces a layer of abstraction allowing for lightweight task scheduling and performance optimization.
Since the runtime environment is always present, user can register tasks from any place in his code.
The task registration API allows for building the arbitrary execution DAG capturing all the work-flow interdependencies and avoid unnecessary waits between independent processing branches.

HPX allows for dividing worker threads (described above) into thread pools in order to separate different workloads. Defining the thread pools and assigning resources to them is done with the use of the resource partitioner. Most of the examples in this repository use two HPX thread pools: (1) default thread pool and (2) blocking thread pool. The reason for this separation is the following fact. Since, single worker thread implements task scheduling (processes many tasks) whenever there is a blocking call (for example image capture) within a single task - *the whole worker thread is blocked*. Consequently, all the tasks in his queue are blocked, even though the resource (pinned PU) is available. For that reason we separate the default pool (on which there should not be any blocking calls) from the blocking pool on which we allow and expect the blocking calls.

Natural question to ask is whether we are wasting resources with this separation. Well, the answer is: we might be. Imagine having 8 processing units and giving up 1 for the blocking pool on which a single 3-second-blocking call is repeated in the loop. This leaves 7 PUs for the computation and 1 PU idle most of the time (note: it can be observed in the qt_hpx_opencv example with low requested FPS). The solution to this problem is over-subscription: we assign 8 worker threads (each pinned to 1 PU) to the default thread pool and on top of that oversubscribe one of the PUs with a single thread from the blocking pool. However, one should be careful with over-subscription because in case when the worker thread from blocking thread pool is executing a lot of tasks and only occasionally experiences a blocking call the overhead of having two worker threads pinned to single PU might be detrimental to performance. Summing up, the best solution depends on the application you are developing and HPX allows for different design choices.

## More on benefits of HPX backend for OpenCV

#### Solving competing backends problem

Without HPX backend in OpenCV available, the user developing an HPX-based application using OpenCV functionality, is forced to use one of the existing OpenCV parallel backends (pthreads, TBB, OMP, etc.). However, none of them is compatible with HPX and spawns own threads for parallel work execution. This is against the general HPX architecture and detrimental to the performance. 

#### Giving greater control
With the HPX backend the parallel algorithms from OpenCV will be effectively executed by the HPX parallel_for loop and therefore chunked into HPX tasks and distributed between (existing) HPX worker threads. Thanks to that user can make use of the thread-pool mechanism and choose the specific thread pool on which the OpenCV algorithm will executed and also control the execution policy for this algorithm.

## This repository

### Structure

Code in this repository was also developed during GSOC 2018. It consists of example cpp applications that exemplify usages of OpenCV with the HPX backend, bash scripts to build and benchmark HPX backend implementation, python script for visualizing the results of benchmarks and a set of interesting experiments results. Following is more detailed description of directories present in this repository:

#### `examples/`
1. `hpx_image_load/` - this is a simple application that shows how to create custom thread pool with the use of resource partitioner. Moreover, it uses OpenCV to load image from the drive, transform it to gray-scale and show to the user.
It is important to note that the above mentioned OpenCV operations are scheduled as HPX tasks and as such are executed within the HPX runtime.
1. `hpx_mandelbrot/` - this application generates mandelbrot image by making calls to the HPX parallel_for loop.
1. `opencv_mandelbrot/` - an *important application* because it was extensively used as a benchmark to compare performance of OpenCV with different parallel backends. It creates the mandelbrot image by making calls to cv::parallel_for_() and therefore dependent on the chosen parallel backend for opencv.
1. `hpx_start_stop/` - this example shows how to start and stop HPX runtime arbitrary number of times.
1. `hpx_start_myargv/` - this example shows how to create proper argc and argv parameters within an application. It was created when a start-stop version of HPX backend for OpenCV was considered.
1. `hpx_opencv_webcam/` - this application performs a real-time face recognition of the image from the webcam. It uses separate HPX thread pool for capturing camera image and separate thread pool for processing (face recognition).
1. `qt_with_cmake/` - this example shows how to properly build qt application with cmake (without using QT creator or QMake).
1. `qt_hpx_opencv/` - this is the most advanced of all example applications. It is based on the [MartyCam application](https://github.com/biddisco/MartyCam/tree/GSoC) and my main contribution is changing the existing architecture such that now the application is combining QT threading mechanisms with HPX. This is a GUI application in which user can switch between live motion detection and face recognition. Some of the processing parameters are editable from the GUI and key performance statistics are displayed live to the user.

#### `bash/`
1. `build_hpx.sh`, `build_opencv.sh` - scripts used to automatize building of HPX and OpenCV libraries in different configurations. Useful for benchmarking, since choice of the backend is a build option, therefore a separate build for each backend is required.
1. `run_opencv_mandelbrot.sh` - script used to perform benchmarking of OpenCV with different backends.
1. `run_opencv_test` - script used to run OpenCV performance and unit tests. It runs the tests against two builds and stores the results in the file with prefix determined by the test name, which allows for easy comparison with meld.
1. `run_opencv_dnn.sh` - script used to run the dnn performance test of OpenCV, used for benchmarking different backends. Apart from allowing for parameter sweep it enforces constant CPU frequency ensuring consistency between benchmarks (note: setting the frequency is implemented and tested only with intel_pstate driver)

#### `python/mandelbrot_benchmark/`
1. `mandelbrot_benchmark.py` - this python script is used to process the files generated by `bash/run_opencv_mandelbrot.sh` and visualize them with 2D plots.

#### `experiments/`
This directory contains results from a number of benchmarking experiments with the use of opencv_mandelbrot benchmark. It was run with the following parameter sweeps:
- over workload size (changing the mandelbrot image target size),
- over number of processing threads,
- over the requested nstripes parameter,

Moreover, there the there are results from an OpenCV DNN performance test with results presented in the .html file.

#### `data/models/`
This directory is contains any data required for the applications to run (for example face recognition model parameters).

## Installation

### Dependencies
In order to build all the examples from this repository the following dependencies are necessary:

1. [HPX](http://stellar-group.org/libraries/hpx/docs/)
    * Note: Add the following CMake option while building HPX:
    ```bash
    -DHPX_WITH_CXX11=ON
    ```
1. [OpenCV (built with HPX backend)](https://github.com/Jakub-Golinowski/opencv/tree/hpx_backend). This is the link to my branch of OpenCV repository with the OpenCV backend implemented. In order to build OpenCV with HPX backend perform the following steps:
    * Clone [my fork](https://github.com/Jakub-Golinowski/opencv/tree/hpx_backend) of the OpenCV repository
    * Follow the [build steps of OpenCV](https://docs.opencv.org/master/d7/d9f/tutorial_linux_install.html) but add the following parameters to the cmake command:
    ```bash
    -DWITH_HPX=ON -DHPX_DIR=[1]
    ```
    where [1] is a path to the subdirectory [...]/lib/cmake/HPX/ of the HPX build directory
3. [Boost](https://www.boost.org/)
1. [QT5](http://doc.qt.io/qt-5/gettingstarted.html)

### Building examples

To build the examples in the release mode perform the following steps (*note: you are assumed to start from this repository root direcotry*):
```bash
mkdir -p build/release/
cd build/release/
cmake -DHPX_DIR=[1] -DOpenCV_DIR=[2] -DQt5_DIR=[3] -DBOOST_ROOT=[4] ../../
make
```
where: 
[1] - path to the subdirectory [...]/lib/cmake/HPX/ of the HPX release build directory,
[2] - path to the OpenCV release build directory,
[3] - path to directory containing Qt5Config.cmake,
[4] - path to root boost directory

Note: you can speed up the build by using `make -j [n]` where [n] denotes the number of threads that should be used for building.

## Interesting follow-ups

1. Going one step further than shared-memory parallelism and executing OpenCV algorithms on cluster of machines with the use of HPX.
1. HPX is capable of executing arbitrarily many nested parallel_for loops, however the OpenCV does not allow for that. It would be interesting to investigate whether this restriction is required for algorithms' correctness or it is just an implementation issue. If the latter is true then it is worth exploring the possibility of unlocking nested cv::parallel_for_() loops with HPX backend.


[HPX_backend_PR]: <https://github.com/opencv/opencv/pull/11897>