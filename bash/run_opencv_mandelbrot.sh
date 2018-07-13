#/bin/bash

# this script is used to build opencv_mandelbrot application  with different parallel backends and run them with measuring the execution time.

### ===========================================================
###                         INCLUDES
### ===========================================================
# source ...

### ===========================================================
###                        INPUT PARAMETERS
### ===========================================================
# Default parameter values
MESSAGE="-- no message provided --"
# Path to the opencv build in which all the backends are built.
OPENCV_BUILD_PATH="/home/jakub/opencv_repo/build/"
# Path to the hpx built dir in which hpx backend is built
HPX_BUILD_PATH="/home/jakub/hpx_repo/build/hpx_11/"
#Below default paths are relative to repository root directory 
LOGS_PATH="./logs/"
APP_PATH="./examples/opencv_mandelbrot/"
REPO_ROOT_PATH=$(cd ../; pwd)/

POSITIONAL=()
while [[ $# -gt 0 ]]
do
key="$1"

case $key in
    -mes|--message)
        MESSAGE="$2"
        shift # past argument
        shift # past value
    ;;
    -ap|--app-path)
        APP_PATH="$2"
        shift; shift
    ;;
    -hp|--hpx-path)
        HPX_BUILD_PATH="$2"
        shift; shift
    ;;
    -op|--opencv-path)
        OPENCV_BUILD_PATH="$2"
        shift; shift
    ;;
    -lp|--logs-path)
        LOGS_PATH="$2"
        shift; shift
    ;;
    *)    # unknown option
        POSITIONAL+=("$1") # save it in an array for later
        shift # past argument
    ;;
esac
done
set -- "${POSITIONAL[@]}" # restore positional parameters

### ===========================================================
###                     FUNCTIONS
### ===========================================================
function build_app {
    local OpenCV_DIR=$1
    local HPX_DIR=$2
    local BACKEND_STARTSTOP=$3
    local BACKEND_NON_HPX=$4
    local mode=$5

    if [ ${mode} = "debug" ]; then
        build_type="Debug"
    elif [ ${mode} = "release" ]; then
        build_type=Release
    else
        echo "ERROR: WRONG mode = ${mode}"
        exit 1
    fi

    echo -e "======================================="
    echo -e "====== BUILDING THE APPLICATION ======="
    echo -e "======================================="
    echo -e "= OpenCV_DIR = ${OpenCV_DIR}"
    echo -e "= HPX_DIR = ${HPX_DIR}"
    echo -e "= BACKEND_STARTSTOP = ${BACKEND_STARTSTOP}"
    echo -e "= BACKEND_NON_HPX = ${BACKEND_NON_HPX}"
    echo -e "= mode = ${mode}"
    echo -e "======================================="
    #change to repo root
    cd ${REPO_ROOT_PATH}${APP_PATH}

    echo "Removing the ${APP_PATH}build directory"
    rm -r build
    echo "Creating the ${APP_PATH}build directory"
    mkdir build
    cd build

    cmake -DCMAKE_BUILD_TYPE=${build_type} -DOpenCV_DIR=${OpenCV_DIR} -DHPX_DIR=${HPX_DIR} \
          -DBACKEND_STARTSTOP=${BACKEND_STARTSTOP} \
          -DBACKEND_NON_HPX=${BACKEND_NON_HPX} \
          ../
    make -j7
    echo -e "++++++++++++++++++++++++++++++++++\n"
}


function run_mandelbrot_app_over_worksize {
    local base_height=$1
    local base_width=$2
    local num_loops=$3
    local num_repetitions=$4
    local backend=$5
    local num_pus=$6
    local sequential=$7

    if [ "${sequential}" = true ]; then
        local logfile=${REPO_ROOT_PATH}${LOGS_PATH}${backend}-seq_baseline-mandelbrot_over_workload.log
    else
        local logfile=${REPO_ROOT_PATH}${LOGS_PATH}${backend}-mandelbrot_over_workload.log
    fi

    
    touch ${logfile}

    echo -e "\n===== RUNNING THE APPLICATION WITH $5 BACKEND (over workload size) ====="

    for (( scaling=1; scaling<=$num_loops; scaling++ )); do
        for ((rep=1; rep<=num_repetitions; rep++)); do
            height=$(($scaling * $base_height))
            width=$(($scaling * $base_width))

            echo "Executing mandelbrot_app with height=$height | width=$width | num_pus = ${num_pus} | sequential = ${sequential}"
            ./opencv_mandelbrot -h $height -w $width -b $backend --hpx:threads ${num_pus} -s ${sequential} >> ${logfile}
            echo "Moving images to the logs directory"
            mv *.png ${REPO_ROOT_PATH}${LOGS_PATH}    
            echo -e "\n"    
        done
    done

    echo "++++++++++++++++++++++++++++++++++"
}

function run_mandelbrot_app_over_num_pus {
    local height=$1
    local width=$2
    local num_repetitions=$3
    local backend=$4
    local max_num_pus=$5
    local sequential=$6

    if [ "${sequential}" = true ]; then
        local logfile=${REPO_ROOT_PATH}${LOGS_PATH}${backend}-seq_baseline-mandelbrot_over_num_pus.log
    else
        local logfile=${REPO_ROOT_PATH}${LOGS_PATH}${backend}-mandelbrot_over_num_pus.log
    fi

    touch ${logfile}

    echo -e "\n===== RUNNING THE APPLICATION WITH ${backend} BACKEND (over num pus) ====="

    for (( num_pus=1; num_pus<=max_num_pus; num_pus++ )); do
        for ((rep=1; rep<=num_repetitions; rep++)); do
            
            echo "Executing mandelbrot_app with height=$height | width=$width | num_pus = ${num_pus} | sequential = ${sequential}"
            ./opencv_mandelbrot -h $height -w $width -b $backend --hpx:threads ${num_pus} -s ${sequential} >> ${logfile}
            echo "Moving images to the logs directory"
            mv *.png ${REPO_ROOT_PATH}${LOGS_PATH}    
            echo ""
        done
    done

    echo "++++++++++++++++++++++++++++++++++"
}

function run_mandelbrot_app_over_nstripes {
    local height=$1
    local width=$2
    local num_repetitions=$3
    local backend=$4
    local num_pus=$5
    local mandelbrot_iter=$6
    local nstripes_start=$7
    local nstripes_stop=$8
    local nstripes_step=$9

    local logfile=${REPO_ROOT_PATH}${LOGS_PATH}${backend}-mandelbrot_over_nstripes.log
    touch ${logfile}

    echo -e "\n===== RUNNING THE APPLICATION WITH ${backend} BACKEND (over nstripes) ====="

    for nstripes in `seq ${nstripes_start} ${nstripes_step} ${nstripes_stop}`; do
        for ((rep=1; rep<=num_repetitions; rep++)); do
            
            echo "Executing mandelbrot_app with height=$height | width=$width | num_pus = ${num_pus} | nstripes = ${nstripes}"
            ./opencv_mandelbrot -h $height -w $width -b $backend --hpx:threads ${num_pus} -i ${mandelbrot_iter} -n ${nstripes}>> ${logfile}
            echo "Moving images to the logs directory"
            mv *.png ${REPO_ROOT_PATH}${LOGS_PATH}    
            echo ""
        done
    done

    echo "++++++++++++++++++++++++++++++++++"
}
### ===========================================================
###                         MAIN
### ===========================================================

TIMESTAMP=$(date +"%Y-%m-%d-%H.%M")

LOGS_PATH=${LOGS_PATH}${TIMESTAMP}/
mkdir ${REPO_ROOT_PATH}${LOGS_PATH}

echo -e "Executing script: ${0}\n"
echo "Used parameters:"
echo "    MESSAGE = ${MESSAGE}"
echo "    LOGS_PATH = ${LOGS_PATH}"
echo "    APP_PATH = ${APP_PATH}"
echo -e "    REPO_ROOT_PATH = ${REPO_ROOT_PATH}\n"


echo "============================================================================="
echo "============================================================================="
echo "===================     Iterating over workload size     ===================="
echo "============================================================================="
echo "============================================================================="
# build mode for the experiment
mode="release"

# number of repetitions
num_reps=3
# base height of the generated image
base_height=480
# base width of the generated image
base_width=540
# number of times the base image is scaled
num_scalings=10
# number of processing units used in the experiment
num_pus=4
# should the run be sequential
sequential=false

echo "Heating up the cores with unlogged run."
build_app ${OPENCV_BUILD_PATH}hpx/${mode} ${HPX_BUILD_PATH}${mode}/lib/cmake/HPX OFF OFF ${mode}
./opencv_mandelbrot -h 1200 -w 1350 -b "heat-up_run"

echo "Running sequential baseline over workload size"
build_app ${OPENCV_BUILD_PATH}pthreads/${mode} ${HPX_BUILD_PATH}${mode}/lib/cmake/HPX ON ON ${mode}
run_mandelbrot_app_over_worksize 480 540 ${num_scalings} ${num_reps} "pthreads" ${num_pus} true

build_app ${OPENCV_BUILD_PATH}hpx/${mode} ${HPX_BUILD_PATH}${mode}/lib/cmake/HPX OFF OFF ${mode}
run_mandelbrot_app_over_worksize 480 540 ${num_scalings} ${num_reps} "hpx" ${num_pus} ${sequential}

build_app ${OPENCV_BUILD_PATH}hpx_startstop/${mode} ${HPX_BUILD_PATH}${mode}/lib/cmake/HPX ON OFF ${mode}
run_mandelbrot_app_over_worksize 480 540 ${num_scalings} ${num_reps} "hpx_startstop" ${num_pus} ${sequential}

build_app ${OPENCV_BUILD_PATH}tbb/${mode} ${HPX_BUILD_PATH}${mode}/lib/cmake/HPX ON ON ${mode}
run_mandelbrot_app_over_worksize 480 540 ${num_scalings} ${num_reps} "tbb" ${num_pus} ${sequential}

build_app ${OPENCV_BUILD_PATH}tbb/${mode} ${HPX_BUILD_PATH}${mode}/lib/cmake/HPX ON ON ${mode}
run_mandelbrot_app_over_worksize 480 540 ${num_scalings} ${num_reps} "omp" ${num_pus} ${sequential}

build_app ${OPENCV_BUILD_PATH}pthreads/${mode} ${HPX_BUILD_PATH}${mode}/lib/cmake/HPX ON ON ${mode}
run_mandelbrot_app_over_worksize 480 540 ${num_scalings} ${num_reps} "pthreads" ${num_pus} ${sequential}

echo "============================================================================="
echo "============================================================================="
echo "===================     Iterating over number of pus     ===================="
echo "============================================================================="
echo "============================================================================="
# as above
mode="release"

# height of the image
height=$((480*10))
# width of the image
width=$((540*10))
# number of repetitions of the experiment
num_reps=3
# maximum number of processing units that will be used in the experiment
max_num_pus=8
# as above
sequential=false

echo "Running sequential baseline over number of PUs."
build_app ${OPENCV_BUILD_PATH}pthreads/${mode} ${HPX_BUILD_PATH}${mode}/lib/cmake/HPX ON ON ${mode}
run_mandelbrot_app_over_num_pus ${height} ${width} ${num_reps} "pthreads" ${max_num_pus} true

build_app ${OPENCV_BUILD_PATH}hpx/${mode} ${HPX_BUILD_PATH}${mode}/lib/cmake/HPX OFF OFF ${mode}
run_mandelbrot_app_over_num_pus ${height} ${width} ${num_reps} "hpx" ${max_num_pus} ${sequential}

build_app ${OPENCV_BUILD_PATH}hpx_startstop/${mode} ${HPX_BUILD_PATH}${mode}/lib/cmake/HPX ON OFF ${mode}
run_mandelbrot_app_over_num_pus ${height} ${width} ${num_reps} "hpx_startstop" ${max_num_pus} ${sequential}

build_app ${OPENCV_BUILD_PATH}tbb/${mode} ${HPX_BUILD_PATH}${mode}/lib/cmake/HPX ON ON ${mode}
run_mandelbrot_app_over_num_pus ${height} ${width} ${num_reps} "tbb" ${max_num_pus} ${sequential}

build_app ${OPENCV_BUILD_PATH}tbb/${mode} ${HPX_BUILD_PATH}${mode}/lib/cmake/HPX ON ON ${mode}
run_mandelbrot_app_over_num_pus ${height} ${width} ${num_reps} "omp" ${max_num_pus} ${sequential}

build_app ${OPENCV_BUILD_PATH}pthreads/${mode} ${HPX_BUILD_PATH}${mode}/lib/cmake/HPX ON ON ${mode}
run_mandelbrot_app_over_num_pus ${height} ${width} ${num_reps} "pthreads" ${max_num_pus} ${sequential}

echo "============================================================================="
echo "============================================================================="
echo "================== Iterating over nstripes (small range) ===================="
echo "============================================================================="
echo "============================================================================="
# as above
mode="release"
height=$((480*10))
width=$((540*10))
num_reps=3
# number of processing units used in the experiment
num_pus=4
#number of internal mandelbrot iterations for each body of parallel_for loop
mandelbrot_iter=10
# variables to setup the sweep range and granularity
nstripes_start="-1"
nstripes_stop=$((100))
nstripes_step=$((1))

build_app ${OPENCV_BUILD_PATH}hpx/${mode} ${HPX_BUILD_PATH}${mode}/lib/cmake/HPX OFF OFF ${mode}
run_mandelbrot_app_over_nstripes ${height} ${width} ${num_reps} "hpx" ${num_pus} ${mandelbrot_iter} ${nstripes_start} ${nstripes_stop} ${nstripes_step}

build_app ${OPENCV_BUILD_PATH}hpx_startstop/${mode} ${HPX_BUILD_PATH}${mode}/lib/cmake/HPX ON OFF ${mode}
run_mandelbrot_app_over_nstripes ${height} ${width} ${num_reps} "hpx_startstop" ${num_pus} ${mandelbrot_iter} ${nstripes_start} ${nstripes_stop} ${nstripes_step}

build_app ${OPENCV_BUILD_PATH}tbb/${mode} ${HPX_BUILD_PATH}${mode}/lib/cmake/HPX ON ON ${mode}
run_mandelbrot_app_over_nstripes ${height} ${width} ${num_reps} "tbb" ${num_pus} ${mandelbrot_iter} ${nstripes_start} ${nstripes_stop} ${nstripes_step}

build_app ${OPENCV_BUILD_PATH}tbb/${mode} ${HPX_BUILD_PATH}${mode}/lib/cmake/HPX ON ON ${mode}
run_mandelbrot_app_over_nstripes ${height} ${width} ${num_reps} "omp" ${num_pus} ${mandelbrot_iter} ${nstripes_start} ${nstripes_stop} ${nstripes_step}

build_app ${OPENCV_BUILD_PATH}pthreads/${mode} ${HPX_BUILD_PATH}${mode}/lib/cmake/HPX ON ON ${mode}
run_mandelbrot_app_over_nstripes ${height} ${width} ${num_reps} "pthreads" ${num_pus} ${mandelbrot_iter} ${nstripes_start} ${nstripes_stop} ${nstripes_step}

# echo "============================================================================="
# echo "============================================================================="
# echo "================== Iterating over nstripes (full range) ===================="
# echo "============================================================================="
# echo "============================================================================="
# as above
# nstripes_start="-1"
# nstripes_stop=$((${height} * ${width}))
# nstripes_step=$(((${nstripes_stop}-${nstripes_start})/50))

# build_app ${OPENCV_BUILD_PATH}hpx/${mode} ${HPX_BUILD_PATH}${mode}/lib/cmake/HPX OFF OFF ${mode}
# run_mandelbrot_app_over_nstripes ${height} ${width} ${num_reps} "hpx" ${num_pus} ${mandelbrot_iter} ${nstripes_start} ${nstripes_stop} ${nstripes_step}

# build_app ${OPENCV_BUILD_PATH}hpx_startstop/${mode} ${HPX_BUILD_PATH}${mode}/lib/cmake/HPX ON OFF ${mode}
# run_mandelbrot_app_over_nstripes ${height} ${width} ${num_reps} "hpx_startstop" ${num_pus} ${mandelbrot_iter} ${nstripes_start} ${nstripes_stop} ${nstripes_step}

# build_app ${OPENCV_BUILD_PATH}tbb/${mode} ${HPX_BUILD_PATH}${mode}/lib/cmake/HPX ON ON ${mode}
# run_mandelbrot_app_over_nstripes ${height} ${width} ${num_reps} "tbb" ${num_pus} ${mandelbrot_iter} ${nstripes_start} ${nstripes_stop} ${nstripes_step}

# build_app ${OPENCV_BUILD_PATH}tbb/${mode} ${HPX_BUILD_PATH}${mode}/lib/cmake/HPX ON ON ${mode}
# run_mandelbrot_app_over_nstripes ${height} ${width} ${num_reps} "omp" ${num_pus} ${mandelbrot_iter} ${nstripes_start} ${nstripes_stop} ${nstripes_step}

# build_app ${OPENCV_BUILD_PATH}pthreads/${mode} ${HPX_BUILD_PATH}${mode}/lib/cmake/HPX ON ON ${mode}
# run_mandelbrot_app_over_nstripes ${height} ${width} ${num_reps} "pthreads" ${num_pus} ${mandelbrot_iter} ${nstripes_start} ${nstripes_stop} ${nstripes_step}
