#/bin/bash

# this script is used to build opencv_mandelbrot application  with different parallel backends and run them with measuring the execution time.

### ===========================================================
### 						INCLUDES
### ===========================================================
# source ./common_functions.sh

### ===========================================================
###						INPUT PARAMETERS
### ===========================================================
# Default parameter values
MESSAGE="-- no message provided --"
#Paths are relative to repository root directory
LOGS_PATH="./logs/"
APP_PATH="./opencv_mandelbrot/"
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
###         			FUNCTIONS
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
	echo -e "\n===== BUILDING THE APPLICATION ====="
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
	echo "++++++++++++++++++++++++++++++++++"
}


function run_mandelbrot_app_over_worksize {
	local base_height=$1
	local base_width=$2
	local num_loops=$3
	local num_repetitions=$4
	local backend=$5
	local num_pus=$6


	local logfile=${REPO_ROOT_PATH}${LOGS_PATH}${backend}-mandelbrot_over_workload.log
	touch ${logfile}

	echo -e "\n===== RUNNING THE APPLICATION WITH $5 BACKEND (over workload size) ====="

	for (( scaling=1; scaling<=$num_loops; scaling++ )); do
		for ((rep=1; rep<=num_repetitions; rep++)); do
			height=$(($scaling * $base_height))
			width=$(($scaling * $base_width))

			echo "Executing mandelbrot_app with height=$height | width=$width | num_pus = ${num_pus}"
			./opencv_mandelbrot -h $height -w $width -b $backend --hpx:threads ${num_pus} >> ${logfile}
			echo -e "\n"

			echo "Moving images to the logs directory"
			mv *.png ${REPO_ROOT_PATH}${LOGS_PATH}	
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

	local logfile=${REPO_ROOT_PATH}${LOGS_PATH}${backend}-mandelbrot_over_num_pus.log
	touch ${logfile}

	echo -e "\n===== RUNNING THE APPLICATION WITH ${backend} BACKEND (over num pus) ====="

	for (( num_pus=1; num_pus<=max_num_pus; num_pus++ )); do
		for ((rep=1; rep<=num_repetitions; rep++)); do
			
			echo "Executing mandelbrot_app with height=$height | width=$width | num_pus = ${num_pus}"
			./opencv_mandelbrot -h $height -w $width -b $backend --hpx:threads ${num_pus} >> ${logfile}
			echo -e "\n"

			echo "Moving images to the logs directory"
			mv *.png ${REPO_ROOT_PATH}${LOGS_PATH}	
		done
	done

	echo "++++++++++++++++++++++++++++++++++"
}
### ===========================================================
### 						MAIN
### ===========================================================

TIMESTAMP=$(date +"%Y-%m-%d-%H.%M")

LOGS_PATH=${LOGS_PATH}${TIMESTAMP}/
mkdir ${REPO_ROOT_PATH}${LOGS_PATH}

echo "Executing script: ${0}"
echo "Used parameters:"
echo "	MESSAGE = ${MESSAGE}"
echo "	LOGS_PATH = ${LOGS_PATH}"
echo "	APP_PATH = ${APP_PATH}"
echo "	REPO_ROOT_PATH = ${REPO_ROOT_PATH}"

echo "============================================================================="
echo "============================================================================="
echo "===================     Iterating over workload size     ===================="
echo "============================================================================="
echo "============================================================================="
mode="release"

num_reps=3
num_scalings=10
num_pus=8

build_app /home/jakub/opencv_repo/build/hpx/${mode} /home/jakub/hpx_repo/build/hpx_11/${mode}/lib/cmake/HPX OFF OFF ${mode}
echo "Heating up the cores with unlogged run."
./opencv_mandelbrot -h 1200 -w 1350 -b "heat-up_run"
run_mandelbrot_app_over_worksize 480 540 ${num_scalings} ${num_reps} "hpx" ${num_pus}

build_app /home/jakub/opencv_repo/build/hpx_nstripes/${mode} /home/jakub/hpx_repo/build/hpx_11/${mode}/lib/cmake/HPX OFF OFF ${mode}
run_mandelbrot_app_over_worksize 480 540 ${num_scalings} ${num_reps} "hpx_nstripes" ${num_pus}

build_app /home/jakub/opencv_repo/build/hpx_startstop/${mode} /home/jakub/hpx_repo/build/hpx_11/${mode}/lib/cmake/HPX ON OFF ${mode}
run_mandelbrot_app_over_worksize 480 540 ${num_scalings} ${num_reps} "hpx_startstop" ${num_pus}

build_app /home/jakub/opencv_repo/build/hpx_nstripes_startstop/${mode} /home/jakub/hpx_repo/build/hpx_11/${mode}/lib/cmake/HPX ON OFF ${mode}
run_mandelbrot_app_over_worksize 480 540 ${num_scalings} ${num_reps} "hpx_nstripes_startstop" ${num_pus}

build_app /home/jakub/opencv_repo/build/tbb/${mode} /home/jakub/hpx_repo/build/hpx_11/${mode}/lib/cmake/HPX OFF ON ${mode}
run_mandelbrot_app_over_worksize 480 540 ${num_scalings} ${num_reps} "tbb" ${num_pus}

build_app /home/jakub/opencv_repo/build/pthreads/${mode} /home/jakub/hpx_repo/build/hpx_11/${mode}/lib/cmake/HPX OFF ON ${mode}
run_mandelbrot_app_over_worksize 480 540 ${num_scalings} ${num_reps} "pthreads" ${num_pus}

echo "============================================================================="
echo "============================================================================="
echo "===================     Iterating over workload size     ===================="
echo "============================================================================="
echo "============================================================================="

mode="release"

height=$((480*4))
width=$((540*4))
num_reps=3
max_num_pus=8

build_app /home/jakub/opencv_repo/build/hpx/${mode} /home/jakub/hpx_repo/build/hpx_11/${mode}/lib/cmake/HPX OFF OFF ${mode}
run_mandelbrot_app_over_num_pus ${height} ${width} ${num_reps} "hpx" ${max_num_pus}

build_app /home/jakub/opencv_repo/build/hpx_nstripes/${mode} /home/jakub/hpx_repo/build/hpx_11/${mode}/lib/cmake/HPX OFF OFF ${mode}
run_mandelbrot_app_over_num_pus ${height} ${width} ${num_reps} "hpx_nstripes" ${max_num_pus}

build_app /home/jakub/opencv_repo/build/hpx_startstop/${mode} /home/jakub/hpx_repo/build/hpx_11/${mode}/lib/cmake/HPX ON OFF ${mode}
run_mandelbrot_app_over_num_pus ${height} ${width} ${num_reps} "hpx_startstop" ${max_num_pus}

build_app /home/jakub/opencv_repo/build/hpx_nstripes_startstop/${mode} /home/jakub/hpx_repo/build/hpx_11/${mode}/lib/cmake/HPX ON OFF ${mode}
run_mandelbrot_app_over_num_pus ${height} ${width} ${num_reps} "hpx_nstripes_startstop" ${max_num_pus}

# build_app /home/jakub/opencv_repo/build/tbb/${mode} /home/jakub/hpx_repo/build/hpx_11/${mode}/lib/cmake/HPX OFF ON ${mode}
# run_mandelbrot_app_over_num_pus ${height} ${width} ${num_reps} "tbb" ${max_num_pus}

# build_app /home/jakub/opencv_repo/build/pthreads/${mode} /home/jakub/hpx_repo/build/hpx_11/${mode}/lib/cmake/HPX OFF ON ${mode}
# run_mandelbrot_app_over_num_pus ${height} ${width} ${num_reps} "pthreads" ${max_num_pus}
