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
function build_opencv {
	local OpenCV_DIR=$1
	local HPX_DIR=$2
	
	echo -e "\n===== BUILDING THE APPLICATION ====="
	#change to repo root
	cd ${REPO_ROOT_PATH}${APP_PATH}

	echo "Removing the ${APP_PATH}build directory"
	rm -r build
	echo "Creating the ${APP_PATH}build directory"
	mkdir build
	cd build

	cmake -DCMAKE_BUILD_TYPE=Debug -DOpenCV_DIR=$1 -DHPX_DIR=$2 ../
	make -j7
	echo "++++++++++++++++++++++++++++++++++"
}

function run_mandelbrot_app {
	local base_height=$1
	local base_width=$2
	local num_loops=$3
	local num_repetitions=$4
	local backend=$5

	local logfile=${REPO_ROOT_PATH}${LOGS_PATH}${TIMESTAMP}-${backend}-mandelbrot.log

	touch ${logfile}

	echo -e "\n===== RUNNING THE APPLICATION WITH $5 BACKEND ====="

	for (( scaling=1; scaling<=$num_loops; scaling++ )); do
		for ((rep=1; rep<=num_repetitions; rep++)); do
			height=$(($scaling * $base_height))
			width=$(($scaling * $base_width))

			echo "Executing mandelbrot_app with height=$height and width=$width"
			./opencv_mandelbrot -h $height -w $width >> ${logfile}
			echo -e "\n"	
		done
	done

	echo "++++++++++++++++++++++++++++++++++"
}
### ===========================================================
### 						MAIN
### ===========================================================

TIMESTAMP=$(date +"%Y-%m-%d-%H.%M")

echo "Executing script: ${0}"
echo "Used parameters:"
echo "	MESSAGE = ${MESSAGE}"
echo "	LOGS_PATH = ${LOGS_PATH}"
echo "	APP_PATH = ${APP_PATH}"
echo "	REPO_ROOT_PATH = ${REPO_ROOT_PATH}"

build_opencv /home/jakub/opencv_repo/build/hpx/debug /home/jakub/hpx_repo/build/hpx_11/debug/lib/cmake/HPX
echo "Heating up the cores with unlogged run."
./opencv_mandelbrot -h 1200 -w 1350
#run the experiment
run_mandelbrot_app 480 540 10 3 "hpx"

build_opencv /home/jakub/opencv_repo/build/tbb/debug /home/jakub/hpx_repo/build/hpx_11/debug/lib/cmake/HPX
#run the experiment
run_mandelbrot_app 480 540 10 3 "tbb"

build_opencv /home/jakub/opencv_repo/build/pthreads/debug /home/jakub/hpx_repo/build/hpx_11/debug/lib/cmake/HPX
#run the experiment
run_mandelbrot_app 480 540 10 3 "pthreads"
