#/bin/bash

# This script is used to run opencv_tests and log the resutls

### ===========================================================
###                         INCLUDES
### ===========================================================
# source ...

### ===========================================================
###                     INPUT PARAMETERS
### ===========================================================
# Default parameter values
MESSAGE="-- no message provided --"
#Paths are relative to repository root directory
LOGS_PATH="logs/"
OPENCV_REPO_PATH=$(eval echo "~/opencv_repo/")
OPENCV_PACKAGE_PATH=$(eval echo "~/packages/opencv/master-2018.07.03/")
OPENCV_TEST_DATA_PATH=$(eval echo "~/opencv_extra/")
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
        -orp|--opencv-repo-path)
        OPENCV_REPO_PATH="$2"
        shift; shift
    ;;
        -opp|--opencv-package-path)
        OPENCV_PACKAGE_PATH="$2"
        shift; shift
    ;;
        -otp|--opencv-test-path)
        OPENCV_TEST_DATA_PATH="$2"
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
function compare_tests_2_backends {
    local module_name=$1
    local test_args=$2
    local backend_1_name=$3
    local backend_1_path=$4
    local backend_2_name=$5
    local backend_2_path=$6
    local logs_dir=$7


    echo -e "\nExecuting compare_tests_2_backends $@"
    run_test ${module_name} ${backend_1_name} ${backend_1_path} ${logs_dir} "${test_args}"
    run_test ${module_name} ${backend_2_name} ${backend_2_path} ${logs_dir} "${test_args}"
}

function run_test {
    local module_name=$1
    local backend_name=$2
    local backend_path=$3
    local logs_dir=$4
    local test_args=$5

    echo -e "\nExecuting run_test $@"
    eval cd ${backend_path}
    echo -e "Entering directory `pwd`"
    echo -e "Executing accuracy test for module ${module_name} using backend ${backend_name} with args ${test_args}"
    echo -e "Log is saved to ${logs_dir}test_${module_name}_${backend_name}.log ..."
    ./opencv_test_${module_name} ${test_args} &> ${logs_dir}test_${module_name}_${backend_name}.log
}
### ===========================================================
###                          MAIN
### ===========================================================

TIMESTAMP=$(date +"%Y-%m-%d-%H.%M")

echo "Executing script: ${0}"
echo "Used parameters:"
echo "        MESSAGE = ${MESSAGE}"
echo "        LOGS_PATH = ${LOGS_PATH}"
echo "        OPENCV_REPO_PATH = ${OPENCV_REPO_PATH}"
echo "        OPENCV_PACKAGE_PATH = ${OPENCV_PACKAGE_PATH}"
echo "        OPENCV_TEST_DATA_PATH = ${OPENCV_TEST_DATA_PATH}"
echo "        REPO_ROOT_PATH = ${REPO_ROOT_PATH}"
echo -e ""

logs_dir=${REPO_ROOT_PATH}${LOGS_PATH}${TIMESTAMP}-unit-test/
echo -e "\nCreating logs directory ${logs_dir}"
eval mkdir -p ${logs_dir}

echo -e "\nSetting environment variable OPENCV_TEST_DATA_PATH to ${OPENCV_TEST_DATA_PATH}"
eval export OPENCV_TEST_DATA_PATH=${OPENCV_TEST_DATA_PATH}

hpx_path=${OPENCV_REPO_PATH}build/hpx/release/bin/
pthreads_path=${OPENCV_REPO_PATH}build/pthreads/release/bin/
package_path=${OPENCV_PACKAGE_PATH}/

declare -A modules_args
modules_args[calib3d]=None
modules_args[core]="--gtest_filter=-Core_globbing.accuracy:OCL_Arithm/Compare.Mat/*:OCL_Arithm/Compare.Scalar/*:OCL_Arithm/Compare.Scalar2/*:OCL_Arithm/Pow.Mat/*"
modules_args[dnn]="--gtest_filter=-Layer_Test_BatchNorm.Accuracy"
modules_args[features2d]=None
modules_args[flann]=None
modules_args[highgui]=None
modules_args[imgcodecs]="--gtest_filter=-Resize.Area_half:Imgcodecs_Tiff_Modes.write_multipage"
modules_args[imgproc]=None
modules_args[ml]="--gtest_filter=-ML_ANN_METHOD.Test/*"
modules_args[objdetect]=None
modules_args[photo]=None
modules_args[shape]=None
modules_args[stitching]=None
modules_args[superres]=None
modules_args[video]=None
modules_args[videoio]=None
modules_args[videostab]=None

echo -e ""
for module_name in "${!modules_args[@]}"; do
    echo -e "\n======================================================================================================"
    printf "Comparing backends for module %s with args: \"%s\"\n" "$module_name" "${modules_args[$module_name]}"
    compare_tests_2_backends ${module_name} ${modules_args[$module_name]} "hpx" ${hpx_path} "pthreads" ${pthreads_path} ${logs_dir} 
    echo "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
done

# List of tests binaries:
### Accuracy tests:
# ./opencv_test_calib3d 
# ./opencv_test_core 
# ./opencv_test_dnn
# ./opencv_test_features2d
# ./opencv_test_flann
# ./opencv_test_highgui
# ./opencv_test_imgcodecs
# ./opencv_test_imgproc
# ./opencv_test_ml
# ./opencv_test_objdetect
# ./opencv_test_photo
# ./opencv_test_shape
# ./opencv_test_stitching
# ./opencv_test_superres
# ./opencv_test_video
# ./opencv_test_videoio
# ./opencv_test_videostab

### Performance tests
# ./opencv_perf_calib3d
# ./opencv_perf_core
# ./opencv_perf_dnn
# ./opencv_perf_features2d
# ./opencv_perf_imgcodecs
# ./opencv_perf_imgproc
# ./opencv_perf_objdetect
# ./opencv_perf_photo
# ./opencv_perf_stitching
# ./opencv_perf_superres
# ./opencv_perf_video
# ./opencv_perf_videoio
