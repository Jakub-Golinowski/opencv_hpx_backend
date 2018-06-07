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
	
	echo -e "\n===== BUILDING THE APPLICATION ====="
	#change to repo root
	cd ${REPO_ROOT_PATH}${APP_PATH}

	echo "Removing the ${APP_PATH}build directory"
	rm -r build
	echo "Creating the ${APP_PATH}build directory"
	mkdir build
	cd build

	cmake -DCMAKE_BUILD_TYPE=Debug -DOpenCV_DIR=$1 -DHPX_DIR=$2 -DBACKEND_STARTSTOP=$3 ../
	make -j7
	echo "++++++++++++++++++++++++++++++++++"
}

function run_mandelbrot_app {
	local base_height=$1
	local base_width=$2
	local num_loops=$3
	local num_repetitions=$4
	local backend=$5
	local num_cores=$6

	local logfile=${REPO_ROOT_PATH}${LOGS_PATH}${TIMESTAMP}-${backend}-mandelbrot.log

	touch ${logfile}

	echo -e "\n===== RUNNING THE APPLICATION WITH $5 BACKEND ====="

	for (( scaling=1; scaling<=$num_loops; scaling++ )); do
		for ((rep=1; rep<=num_repetitions; rep++)); do
			height=$(($scaling * $base_height))
			width=$(($scaling * $base_width))

			echo "Executing mandelbrot_app with height=$height and width=$width"
			./opencv_mandelbrot -h $height -w $width -b $backend >> ${logfile}
			echo -e "\n"

			echo "Copying images to the logs directory"
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

mode="release"

build_app /home/jakub/opencv_repo/build/hpx/${mode} /home/jakub/hpx_repo/build/hpx_11/${mode}/lib/cmake/HPX OFF
echo "Heating up the cores with unlogged run."
./opencv_mandelbrot -h 1200 -w 1350
#run the experiment
run_mandelbrot_app 480 540 2 3 "hpx"

build_app /home/jakub/opencv_repo/build/hpx_nstripes/${mode} /home/jakub/hpx_repo/build/hpx_11/${mode}/lib/cmake/HPX OFF
#run the experiment
run_mandelbrot_app 480 540 2 3 "hpx_nstripes"

build_app /home/jakub/opencv_repo/build/hpx_startstop/${mode} /home/jakub/hpx_repo/build/hpx_11/${mode}/lib/cmake/HPX ON
#run the experiment
run_mandelbrot_app 480 540 2 3 "hpx_startstop"

build_app /home/jakub/opencv_repo/build/hpx_nstripes_startstop/${mode} /home/jakub/hpx_repo/build/hpx_11/${mode}/lib/cmake/HPX ON
#run the experiment
run_mandelbrot_app 480 540 2 3 "hpx_nstripes_startstop"

# build_app /home/jakub/opencv_repo/build/tbb/${mode} /home/jakub/hpx_repo/build/hpx_11/${mode}/lib/cmake/HPX
# #run the experiment
# run_mandelbrot_app 480 540 2 3 "tbb"

# build_app /home/jakub/opencv_repo/build/pthreads/${mode} /home/jakub/hpx_repo/build/hpx_11/${mode}/lib/cmake/HPX
# #run the experiment
# run_mandelbrot_app 480 540 2 3 "pthreads"
