#/bin/bash

# this script is used to build OpenCV library with different options

### ===========================================================
###                         INCLUDES
### ===========================================================
# source ./common_functions.sh

### ===========================================================
###                     INPUT PARAMETERS
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
###                        FUNCTIONS
### ===========================================================
function build_opencv {
        local backend_name=$1
        local mode=$2
        local with_startstop=$3
        local non_dyn_main=$4
        

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
            if [ ${non_dyn_main} = "ON" ]; then
                hpx_dir="/home/jakub/hpx_repo/build/hpx_11_non-dynamic_hpx_main/${mode}/lib/cmake/HPX"
            elif [ ${non_dyn_main} = "OFF" ]; then
                hpx_dir="/home/jakub/hpx_repo/build/hpx_11/${mode}/lib/cmake/HPX"
            else
                echo "ERROR: WRONG non_dyn_main = ${non_dyn_main}"
                exit 1
            fi
                cmake -DCMAKE_BUILD_TYPE=${build_type} \
                      -DCMAKE_INSTALL_PREFIX=~/packages/opencv/with_${backend_name}/ \
                      -DWITH_HPX=ON \
                      -DWITH_HPX_STARTSTOP=${with_startstop} \
                      -DHPX_DIR=${hpx_dir} \
                      ../../../
        elif [ ${backend_prefix} = tbb ]; then
                cmake -DCMAKE_BUILD_TYPE=${build_type} \
              -DCMAKE_INSTALL_PREFIX=~/packages/opencv/with_${backend_name}/ \
              -DWITH_TBB=ON \
              -DBUILD_TBB=ON \
              ../../../
        elif [ ${backend_prefix} = "omp" ]; then
                cmake -DCMAKE_BUILD_TYPE=${build_type} \
                          -DWITH_OPENMP=ON \
                  -DCMAKE_INSTALL_PREFIX=~/packages/opencv/with_${backend_name}/ \
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
###                          MAIN
### ===========================================================

TIMESTAMP=$(date +"%Y-%m-%d-%H.%M")

echo "Executing script: ${0}"
echo "Used parameters:"
echo "        MESSAGE = ${MESSAGE}"
echo "        LOGS_PATH = ${LOGS_PATH}"
echo "        OPENCV_PATH = ${OPENCV_PATH}"
echo "        REPO_ROOT_PATH = ${REPO_ROOT_PATH}"


for mode in "debug" "release"; do
    build_opencv "hpx" "${mode}" "OFF" "OFF" 
    build_opencv "hpx_non-dyn_main" "${mode}" "OFF" "ON"
    build_opencv "hpx_startstop" "${mode}" "ON" "OFF" 

    build_opencv "tbb" "${mode}" "NA" "NA" 
    build_opencv "omp" "${mode}" "NA" "NA" 
    build_opencv "pthreads" "${mode}" "NA" "NA" 
done