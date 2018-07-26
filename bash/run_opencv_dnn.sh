#/bin/bash

# This script is used to run the dnn perf test of opencv against different builds
# with the use of opencv runscripts

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
OPENCV_REPO_PATH="${HOME}/opencv_repo/"
OPENCV_TEST_DATA_PATH="${HOME}/opencv_extra/testdata/"
CPU_FREQ=50

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
    *)    # unknown option
            POSITIONAL+=("$1") # save it in an array for later
            shift # past argument
    ;;
esac
done
set -- "${POSITIONAL[@]}" # restore positional parameters

### ===========================================================
###                     OTHER PARAMETERS
### ===========================================================
REPO_ROOT_PATH=$(cd ../; pwd)/
OCV_PY_PATH=${OPENCV_REPO_PATH}/modules/ts/misc
LOGS_DIR_PREFIX=${REPO_ROOT_PATH}logs/

### ===========================================================
###                        FUNCTIONS
### ===========================================================
# Runs the perf_test for the specific module on the specific build
# Assumes python 2.7 env is launched
function run_perf_test {
    local module_name=$1
    local backend_name=$2
    local backend_path=$3
    local logs_path=$4
    local test_args=$5

    local run_timestamp=$(date +"%Y-%m-%d-%H.%M")
    local full_save_path=${logs_path}${run_timestamp}-perf_${module_name}_${backend_name}.xml

    echo -e "================================================================================================================="

    local dstat_full_save_path=${logs_path}${run_timestamp}-perf_${module_name}_${backend_name}_dsts.csv
    echo -e "\nLaunching dstat and logging to ${full_save_path}"
    dstat -t --cpufreq --out ${dstat_full_save_path} -cm > /dev/null &
    dstat_pid=$!
       
    echo -e "\nExecuting run_perf_test with params: \n$@"
    echo -e "\n\trun_perf_test is executed for module ${module_name} using backend ${backend_name} with test_args ${test_args}"
    echo -e "Log is saved to ${full_save_path} ..."
    
    python ./run.py ${backend_path} -t ${module_name} -w ${logs_path} ${test_args}

   
    echo -e "\nRenaming $(ls ${logs_path} | grep '^dnn') to ${full_save_path}"
    mv ${logs_path}dnn_*.xml ${full_save_path}
    
    echo -e "\nKilling dstat"
    kill $dstat_pid
    
    echo -e "=================================================================================================================\n\n"
}

function sweep_milc_mibt {
    local module_name=$1
    local backend_base_name=$2
    local backend_path=$3
    local logs_path=$4
    local test_args=$5
    local milc_vals="$6"
    local mibt_vals="$7"
    local hpx_fixed_options="$8"
    
    for milc in ${milc_vals}; do
        for mibt in ${mibt_vals}; do           
            echo "Current config: mibt = ${mibt} | milc = ${milc}"
            backend_name="${backend_base_name}-${milc}milc-${mibt}mibt"
            test_args="${hpx_fixed_options} --hpx:threads=4 --hpx:ini=hpx.max_idle_loop_count=${milc} --hpx:ini=hpx.max_idle_backoff_time=${mibt} --hpx:dump-config" # --gtest_filter=ConvolutionPerfTest_perf.perf/38"
            run_perf_test ${module_name} ${backend_name} ${backend_path} ${logs_path} "${test_args}"
        done  
    done
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
echo "        CPU_FREQ=${CPU_FREQ}"
echo "        REPO_ROOT_PATH = ${REPO_ROOT_PATH}"
echo "        OCV_PY_PATH = ${OCV_PY_PATH}"
echo "        LOGS_DIR_PREFIX=${LOGS_DIR_PREFIX}"
echo -e ""

#store the original cpufreq boundaries:
original_low_freq_bound=$(cat /sys/devices/system/cpu/intel_pstate/min_perf_pct)
original_hi_freq_bound=$(cat /sys/devices/system/cpu/intel_pstate/max_perf_pct)

echo -e "\n(root required) Setting the CPU frequency to ${CPU_FREQ}% of max boost value (NOTE: intel_pstate driver assumed!)"
echo "${CPU_FREQ}" | sudo tee /sys/devices/system/cpu/intel_pstate/max_perf_pct
echo "$((${CPU_FREQ} - 1))" | sudo tee /sys/devices/system/cpu/intel_pstate/min_perf_pct

logs_path=${LOGS_DIR_PREFIX}${TIMESTAMP}-dnn-test/
echo -e "\nCreating logs directory for tests: ${logs_path}"
eval mkdir -p ${logs_path}

echo -e "\nSetting environment variable OPENCV_TEST_DATA_PATH to ${OPENCV_TEST_DATA_PATH}"
eval export OPENCV_TEST_DATA_PATH=${OPENCV_TEST_DATA_PATH}

echo -e "\nEntering the python scripts directory of opencv ts module"
cd ${OCV_PY_PATH}
echo -e "Contents of the ${OCV_PY_PATH}:"
ls

echo -e "\nActivating conda environment (Python 2.7):"
source activate py27
python --version

hpx_fixed_options="--hpx:ini=hpx.max_background_threads=0 --hpx:ini=hpx.threadpools.io_pool_size=1 --hpx:ini=hpx.threadpools.timer_pool_size=1"
module_name=dnn
  
backend_path="/home/jakub/opencv_repo/build/hpx_non-dyn_main_nonet/release/"
    milc=1000000
    mibt=1000000
    backend_name="hpx-4thr-${milc}milc-${mibt}mibt"
    test_args="${hpx_fixed_options} --hpx:threads=4 --hpx:ini=hpx.max_idle_loop_count=${milc} --hpx:ini=hpx.max_idle_backoff_time=${mibt} --hpx:dump-config" # --gtest_filter=ConvolutionPerfTest_perf.perf/38"
    run_perf_test ${module_name} ${backend_name} ${backend_path} ${logs_path} "${test_args}"

backend_path="/home/jakub/opencv_repo/build/pthreads_master_05.07.18/release"
    backend_name="pthreads_master-4thr"
    test_args="--perf_threads=4"
    run_perf_test ${module_name} ${backend_name} ${backend_path} ${logs_path} "${test_args}"

backend_path="/home/jakub/opencv_repo/build/tbb/release"
    backend_name="tbb-4thr"
    test_args="--perf_threads=4"
    run_perf_test ${module_name} ${backend_name} ${backend_path} ${logs_path} "${test_args}"    

backend_path="/home/jakub/opencv_repo/build/omp/release"
    backend_name="omp-4thr"
    test_args="--perf_threads=4"
    run_perf_test ${module_name} ${backend_name} ${backend_path} ${logs_path} "${test_args}"

echo -e "\n\nCreating summary"
python ./summary.py ${logs_path}*.xml -o html > ${logs_path}${TIMESTAMP}-summary.html

echo -e "\n(root required) Restoring the CPU frequency to the original values (NOTE: intel_pstate driver assumed!):\n\tmin = ${original_low_freq_bound}% of max boost value \n\tmax=${original_hi_freq_bound}% of max boost value"
echo "${original_hi_freq_bound}" | sudo tee /sys/devices/system/cpu/intel_pstate/max_perf_pct
echo "${original_low_freq_bound}" | sudo tee /sys/devices/system/cpu/intel_pstate/min_perf_pct
