#/bin/bash

# This script is used to run perf and accuracy tests of opencv for 2 different builds
# and logs the results for easy comparison of test results for these 2 builds.

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
OPENCV_REPO_PATH="${HOME}/opencv_repo/"
OPENCV_TEST_DATA_PATH="${HOME}/opencv_extra/testdata/"
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
    local is_perf=$8

    echo -e "\nExecuting compare_tests_2_backends $@"
    run_test ${module_name} ${backend_1_name} ${backend_1_path} ${logs_dir} "${test_args}" ${is_perf}
    run_test ${module_name} ${backend_2_name} ${backend_2_path} ${logs_dir} "${test_args}" ${is_perf}
}

function run_test {
    local module_name=$1
    local backend_name=$2
    local backend_path=$3
    local logs_dir=$4
    local test_args=$5
    local is_perf=$6

    echo -e "\nExecuting run_test $@"
    eval cd ${backend_path}
    echo -e "Entering directory `pwd`"
    echo -e "Executing accuracy test for module ${module_name} using backend ${backend_name} with args ${test_args}"
    echo -e "Log is saved to ${logs_dir}test_${module_name}_${backend_name}.log ..."
    if [ ${is_perf} = "ON" ]; then
        ./opencv_perf_${module_name} ${test_args} &> ${logs_dir}perf_${module_name}_${backend_name}.log
    else
        ./opencv_test_${module_name} ${test_args} &> ${logs_dir}test_${module_name}_${backend_name}.log
    fi
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
echo "        OPENCV_TEST_DATA_PATH = ${OPENCV_TEST_DATA_PATH}"
echo "        REPO_ROOT_PATH = ${REPO_ROOT_PATH}"
echo -e ""

logs_dir_test=${REPO_ROOT_PATH}${LOGS_PATH}${TIMESTAMP}-unit-test/
echo -e "\nCreating logs directory for accuracy tests ${logs_dir_test}"
eval mkdir -p ${logs_dir_test}

echo -e "\nSetting environment variable OPENCV_TEST_DATA_PATH to ${OPENCV_TEST_DATA_PATH}"
eval export OPENCV_TEST_DATA_PATH=${OPENCV_TEST_DATA_PATH}

backend_1_path=${OPENCV_REPO_PATH}build/hpx/release/bin/
backend_1_name="hpx"

backend_2_path=${OPENCV_REPO_PATH}build/pthreads_master_05.07.18/release/bin/
backend_2_name="pthreads_master"

declare -A modules_args
modules_args[calib3d]="--gtest_filter=-fisheyeTest.stereoRectify"
modules_args[core]="--gtest_filter=-Core_globbing.accuracy:OCL_Arithm/Compare.Mat/*:OCL_Arithm/Compare.Scalar/*:OCL_Arithm/Compare.Scalar2/*:OCL_Arithm/Pow.Mat/*"
modules_args[dnn]="--gtest_filter=-Layer_Test_BatchNorm.Accuracy"
modules_args[features2d]=None
modules_args[flann]=None
modules_args[highgui]=None
modules_args[imgcodecs]="--gtest_filter=-Resize.Area_half:Imgcodecs_Tiff_Modes.write_multipage"
modules_args[imgproc]="--gtest_filter=-Resize.Area_half:Imgproc_GrabCut.repeatability"
modules_args[ml]="--gtest_filter=-ML_ANN_METHOD.Test/*"
modules_args[objdetect]=None
modules_args[photo]=None
modules_args[shape]=None
modules_args[stitching]=None
modules_args[superres]=None
modules_args[video]=None
modules_args[videoio]=None
modules_args[videostab]=None

echo -e "\n======================================================================================================"
echo -e "======================================================================================================"
echo -e "============================== RUNNING THE ACCURACY TESTS COMPARISIONS ==============================="
echo -e "======================================================================================================"
echo -e "======================================================================================================\n\n"
for module_name in "${!modules_args[@]}"; do
    echo -e "\n======================================================================================================"
    printf "Comparing backends for module %s with args: \"%s\"\n" "$module_name" "${modules_args[$module_name]}"
    compare_tests_2_backends ${module_name} ${modules_args[$module_name]} "hpx" ${backend_1_path} "pthreads" ${backend_2_path} ${logs_dir_test} "OFF"
    echo "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
done

logs_dir_perf=${REPO_ROOT_PATH}${LOGS_PATH}${TIMESTAMP}-perf-test/
echo -e "\nCreating logs directory for accuracy tests ${logs_dir_perf}"
eval mkdir -p ${logs_dir_perf}

declare -A perf_modules_args
perf_modules_args[videoio]="None"
perf_modules_args[calib3d]="None"
perf_modules_args[core]="None"
perf_modules_args[dnn]="None"
perf_modules_args[features2d]="None"
perf_modules_args[imgcodecs]="None"
perf_modules_args[imgproc]="None"
perf_modules_args[objdetect]="None"
perf_modules_args[photo]="None"
perf_modules_args[stitching]="None"
perf_modules_args[superres]="None"
perf_modules_args[video]="None"

echo -e "\n======================================================================================================"
echo -e "======================================================================================================"
echo -e "============================== RUNNING THE PERFORMANCE TESTS COMPARISIONS ============================"
echo -e "======================================================================================================"
echo -e "======================================================================================================\n\n"
for module_name in "${!perf_modules_args[@]}"; do
    echo -e "\n======================================================================================================"
    printf "Comparing backends for module %s with args: \"%s\"\n" "$module_name" "${perf_modules_args[$module_name]}"
    compare_tests_2_backends ${module_name} ${perf_modules_args[$module_name]} "hpx" ${backend_1_path} "pthreads" ${backend_2_path} ${logs_dir_perf} "ON"
    echo "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
done
