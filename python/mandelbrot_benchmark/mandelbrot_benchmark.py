import numpy as np
import matplotlib.pylab as plt
import argparse
from enum import Enum


class Metric(Enum):
    PARALLEL_TIME = 0
    SEQUENTIAL_TIME = 1
    SPEEDUP = 2


def plot_backends_over_key(backend_names, backends_perf_dict,
                            backends_std_dict, metric,
                            title="", xlabel="", ylabel="",
                            save_path=""):
    fig, ax = plt.subplots()

    if isinstance(metric, Metric):
        m = metric.value
        print("Using metric: " + str(Metric(metric)) )
    else:
        print("ERROR: metric is not instance of class Metric: " + str(metric))
        exit(-1)

    for backend_name in backend_names:
        backend_perf_dict = backends_perf_dict[backend_name]
        backend_std_dict = backends_std_dict[backend_name]

        x = backend_perf_dict.keys()
        y = [backend_perf_dict[k][m] for k in backend_perf_dict.keys()]
        y_std =[backend_std_dict[k][m] for k in
                  backend_perf_dict.keys()]

        plt.errorbar(x, y, yerr=y_std,
                     fmt='o-', capsize=3, elinewidth=1,
                     markeredgewidth=1, label=backend_name)

    plt.title(title)
    plt.xlabel(xlabel)
    plt.ylabel(ylabel)
    plt.legend()
    plt.grid(True)

    return fig


def parse_entry(lines, offset):
    # lines - list of lines from log file
    # offset - offset in the logfiles
    #each entry is assumed to have 4 lines

    size_line_tokes = lines[0 + offset].split(" ")
    height = int(size_line_tokes[1].split("=")[1])
    width = int(size_line_tokes[2].split("=")[1])

    parallel_time = float(lines[0 + offset + 1].split(" ")[2])
    sequential_time = float(lines[0 + offset + 2].split(" ")[2])
    speedup = float(lines[0 + offset + 3].split(" ")[1])

    return [height*width, parallel_time, sequential_time, speedup]


if __name__ == "__main__":

    parser = argparse.ArgumentParser()
    parser.add_argument("logs_path",
                        help="Path to the folder with run logs")
    args = parser.parse_args()
    logs_path = args.logs_path

    backends_dict = {}
    backends_devs_dict = {}

    # backend_names = ["hpx", "hpx_startstop", "tbb"]
    backend_names = ["hpx", "hpx_startstop", "tbb"]
    for backend_name in backend_names:

        # Each entry of dict is a list of:
        # #0 parallel time
        # #1 sequential time
        # #2 speedup
        backend_perf_dict = {}
        logfile = logs_path + "/" + backend_name + "-mandelbrot_over_workload.log"

        with open(logfile) as file:
            lines = file.read().splitlines()

            num_entries = round(len(lines)/4)
            for i in range(0, num_entries):
                entry = parse_entry(lines, 4 * i)
                key = entry[0]
                value = entry[1:]
                if key in backend_perf_dict:
                    # backend_perf_dict[key] = np.average([backend_perf_dict[key], value], axis=0)
                    backend_perf_dict[key].append(value)
                else:
                    backend_perf_dict[key] = [value]

        backend_perf_devs_dict = {}
        for key in backend_perf_dict.keys():
            averges = \
                    [float(sum(col)) / len(col) for col
                                                in zip(*backend_perf_dict[key])]
            stds = [np.std(np.asarray(col), axis=0)
                    for col in zip(*backend_perf_dict[key])]
            backend_perf_dict[key] = averges
            backend_perf_devs_dict[key] = stds

        backends_dict[backend_name] = backend_perf_dict
        backends_devs_dict[backend_name] = backend_perf_devs_dict

    fig = plot_backends_over_key(backend_names, backends_dict,
                           backends_devs_dict, Metric.PARALLEL_TIME,
                           title="Parallel processing time as function of "
                                 "image size",
                           xlabel="Number of pixels",
                           ylabel="Processing time [s]")
    plt.show()

    fig = plot_backends_over_key(backend_names, backends_dict,
                           backends_devs_dict, Metric.SEQUENTIAL_TIME,
                           title="Sequential processing time as function of "
                                 "image size",
                           xlabel="Number of pixels",
                           ylabel="Processing time [s]")
    plt.show()

    fig = plot_backends_over_key(backend_names, backends_dict,
                           backends_devs_dict, Metric.SPEEDUP,
                           title="Speed-up as function of "
                                 "image size",
                           xlabel="Number of pixels",
                           ylabel="Speed-up")
    plt.show()