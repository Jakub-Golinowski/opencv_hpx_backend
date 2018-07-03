#!/bin/bash

# this script is used to build HPX library with different options

### ===========================================================
###                         INCLUDES
### ===========================================================
# source ./common_functions.sh

### ===========================================================
###                        INPUT PARAMETERS
### ===========================================================
# Default parameter values
MESSAGE="-- no message provided --"
#Paths are relative to repository root directory
LOGS_PATH="./logs/"
HPX_PATH="/home/jakub/hpx_repo/"
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
    -hp|--hpx-path)
        HPX_PATH="$2"
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
function build_hpx {
    local mode=$1

    echo -e "\n===== BUILDING HPX in ${mode} mode ====="

    echo -e "\nGoing to ${HPX_PATH}build"
    cd ${OPENCV_PATH}build
    echo -e "\nContents of the `pwd`"
    ls

    echo -e "\nCreating the ${mode} directory"
    mkdir ${mode}
    echo -e "\nEntering the ${mode} directory"
    cd ${mode}
    echo -e "\nRunning cmake"

    if [ ${mode} = "debug" ]; then
        build_type="Debug"
    elif [ ${mode} = "release"]; then
        build_type=Release
    else
        echo "ERROR: WRONG mode = ${mode}"
        exit 1
    fi

    echo "Build type: ${build_type}"

    cmake -DCMAKE_BUILD_TYPE=${build_type} \
      -DCMAKE_INSTALL_PREFIX=/home/jakub/packages/hpx \
      -DBOOST_ROOT=/home/jakub/packages/boost/boost_1_65_1 \
      -DHWLOC_ROOT=/home/jakub/packages/hwloc \
      -DTCMALLOC_ROOT=/home/jakub/packages/gperftools \
      -DHPX_WITH_CXX11=On \
      ../../

    echo -e "\nBuilding OpenCV"

    make -j7

    echo "++++++++++++++++++++++++++++++++++"
}
### ===========================================================
###                         MAIN
### ===========================================================

TIMESTAMP=$(date +"%Y-%m-%d-%H.%M")

echo "Executing script: ${0}"
echo "Used parameters:"
echo "    MESSAGE = ${MESSAGE}"
echo "    LOGS_PATH = ${LOGS_PATH}"
echo "    HPX_PATH = ${HPX_PATH}"
echo "    REPO_ROOT_PATH = ${REPO_ROOT_PATH}"

build_hpx "debug"
build_hpx "release"


