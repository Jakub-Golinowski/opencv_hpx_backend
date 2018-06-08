#/bin/bash

# this script is used to build OpenCV library with different options

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
OPENCV_PATH="/home/jakub/opencv_repo/"
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
	-op|--opencv-path)
		OPENCV_PATH="$2"
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
	local backend_name=$1
	local with_nstripes=$2
	local with_startstop=$3
	local mode=$4

	echo -e "\n===== BUILDING OpenCV ${backend_name} in ${mode} mode ====="

	echo -e "\nGoing to ${OPENCV_PATH}build"
	cd ${OPENCV_PATH}build
	echo -e "\nContents of the `pwd`"
	ls

	# echo -e "\nRemoving the ${backend_name} directory"
	#rm -r ${backend_name}
	echo -e "\nCreating the ${backend_name} directory"
	mkdir ${backend_name}
	echo -e "\nEntering the ${backend_name} directory"
	cd ${backend_name}

	echo -e "\nCreating the ${mode} directory"
	mkdir ${mode}
	echo -e "\nEntering the ${mode} directory"
	cd ${mode}
	echo -e "\nRunning cmake"

	if [ ${mode} = "debug" ]; then
		build_type="Debug"
	elif [ ${mode} = "release" ]; then
    	build_type=Release
	else
		echo "ERROR: WRONG mode = ${mode}"
		exit 1
	fi

	echo "Build type: ${build_type}"

	backend_prefix=$(echo ${backend_name} | awk '{print substr($0,0,3)}')

	echo "Backend prefix: ${backend_prefix}"

	if [ ${backend_prefix} = hpx ]; then
		cmake -DCMAKE_BUILD_TYPE=${build_type} \
		      -DCMAKE_INSTALL_PREFIX=~/packages/opencv/with_${backend_name}/ \
		      -DWITH_HPX=ON \
			  -DWITH_HPX_NSTRIPES=$2 \
	          -DWITH_HPX_STARTSTOP=$3 \
		      -DHPX_DIR=/home/jakub/hpx_repo/build/hpx_11/${mode}/lib/cmake/HPX \
		      ../../../
	elif [ ${backend_prefix} = tbb ]; then
		cmake -DCMAKE_BUILD_TYPE=${build_type} \
	      -DCMAKE_INSTALL_PREFIX=~/packages/opencv/with_${backend_name}/ \
	      -DWITH_TBB=ON \
	      -DBUILD_TBB=ON \
	      ../../../
	elif [ ${backend_prefix} = "pth" ]; then
		cmake -DCMAKE_BUILD_TYPE=${build_type} \
	      -DCMAKE_INSTALL_PREFIX=~/packages/opencv/with_${backend_name}/ \
	      ../../../
    fi
	echo -e "\nBuilding OpenCV"      

	make -j7

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
echo "	OPENCV_PATH = ${OPENCV_PATH}"
echo "	REPO_ROOT_PATH = ${REPO_ROOT_PATH}"

mode="debug"
# build_opencv "hpx" "OFF" "OFF" "${mode}"
# build_opencv "hpx_nstripes" "ON" "OFF" "${mode}"
# build_opencv "hpx_nstripes_startstop" "ON" "ON" "${mode}"
# build_opencv "hpx_startstop" "OFF" "ON" "${mode}"
# build_opencv "tbb" "NA" "NA" "${mode}"
# build_opencv "pthreads" "NA" "NA" "${mode}"

mode="release"
build_opencv "hpx" "OFF" "OFF" "${mode}"
build_opencv "hpx_nstripes" "ON" "OFF" "${mode}"
build_opencv "hpx_nstripes_startstop" "ON" "ON" "${mode}"
build_opencv "hpx_startstop" "OFF" "ON" "${mode}"
build_opencv "tbb" "NA" "NA" "${mode}"
build_opencv "pthreads" "NA" "NA" "${mode}"