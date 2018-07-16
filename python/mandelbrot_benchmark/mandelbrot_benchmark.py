import numpy as np
import matplotlib.pylab as plt
import argparse
from enum import Enum
import os
from matplotlib.ticker import EngFormatter
from matplotlib.backends.backend_pdf import PdfPages


class Metric(Enum):
    PARALLEL_TIME = 0
    SEQUENTIAL_TIME = 1
    SPEEDUP = 2


class Sweep(Enum):
    SIZE = 0
    NUM_PUS = 1
    NSTRIPES = 2


NUM_LINES_PER_ENTRY=2
SEQ_BASELINE_BACKEND_NAME= "pthreads-seq_baseline"


def plot_backends_over_key(backend_names, backends_perf_dict,
                            backends_std_dict, metric,
                            title="", xlabel="", ylabel="",
                            save_path=""):
    fig, ax = plt.subplots()

    if isinstance(metric, Metric):
        m = metric.value
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

    ax.xaxis.set_major_formatter(EngFormatter())

    plt.title(title)
    plt.xlabel(xlabel)
    plt.ylabel(ylabel)
    plt.legend()
    plt.grid(True)

    if os.path.isdir(save_path):
        file_name=title.replace(" ", "_") + ".png"
        full_save_path = os.path.join(save_path, file_name)
        print("Saving image to path: " + full_save_path)
        fig.savefig(full_save_path, dpi=300,
                    format='png', bbox_inches='tight')

    return fig


def parse_entry(lines, offset, sweep=Sweep.SIZE):
    # lines - list of lines from log file
    # offset - offset in the logfiles
    #each entry is assumed to have 2 lines

    config_line_tokes = lines[0 + offset].split(" ")
    height = int(config_line_tokes[0].split("=")[1])
    width = int(config_line_tokes[1].split("=")[1])
    backend_name = config_line_tokes[2].split("=")[1]
    num_pus = int(config_line_tokes[3].split("=")[1])
    mandelbrot_iter = int(config_line_tokes[4].split("=")[1])
    nstripes = float(config_line_tokes[5].split("=")[1])
    sequential = bool(config_line_tokes[6].split("=")[1])

    time = float(lines[0 + offset + 1].split(" ")[-2])

    if sweep == Sweep.SIZE:
        return [height*width, time]
    elif sweep == Sweep.NUM_PUS:
        return [num_pus, time]
    elif sweep == Sweep.NSTRIPES:
        return [nstripes, time]
    else:
        print("ERROR: Wrong Sweep: " + str(sweep))
        exit(-1)


def extract_time_single_backend(backend_name, sweep):
    # Each entry of dict is a list of:
    # 0 parallel time
    backend_perf_dict = {}
    backend_std_perf_dict = {}
    logfile = ""
    if sweep == Sweep.SIZE:
        logfile = logs_path + "/" + backend_name + \
                  "-mandelbrot_over_workload.log"
    elif sweep == Sweep.NUM_PUS:
        logfile = logs_path + "/" + backend_name + \
                  "-mandelbrot_over_num_pus.log"
    elif sweep == Sweep.NSTRIPES:
        logfile = logs_path + "/" + backend_name + \
                  "-mandelbrot_over_nstripes.log"
    else:
        print("ERROR: Wrong key: " + str(sweep))
        exit(-1)

    with open(logfile) as file:
        lines = file.read().splitlines()

        num_entries = round(len(lines) / NUM_LINES_PER_ENTRY)
        for i in range(0, num_entries):
            entry = parse_entry(lines, NUM_LINES_PER_ENTRY * i, sweep)
            key = entry[0]
            value = entry[1:]
            if key in backend_perf_dict:
                backend_perf_dict[key].append(value)
            else:
                backend_perf_dict[key] = [value]

    for key in backend_perf_dict.keys():
        averages = \
            [float(sum(col)) / len(col) for col
             in zip(*backend_perf_dict[key])]
        stds = [np.std(np.asarray(col), axis=0)
                for col in zip(*backend_perf_dict[key])]
        backend_perf_dict[key] = averages
        backend_std_perf_dict[key] = stds

    return backend_perf_dict, backend_std_perf_dict


def extract_perf(backend_names, sweep, use_sequential_baseline=True):
    backends_dict = {}
    backends_std_dict = {}

    for backend_name in backend_names:
        backend_perf_dict, backend_std_perf_dict = \
            extract_time_single_backend(backend_name, sweep)

        backends_dict[backend_name] = backend_perf_dict
        backends_std_dict[backend_name] = backend_std_perf_dict

    if(not use_sequential_baseline):
        return backends_dict, backends_std_dict

    seq_baseline_perf_dict, seq_baseline_std_dict = \
        extract_time_single_backend(SEQ_BASELINE_BACKEND_NAME, sweep)

    # Append sequential baseline to backend_perf_dict
    for backend_name in backends_dict:
        for key in backends_dict[backend_name].keys():
            backends_dict[backend_name][key].append(
                seq_baseline_perf_dict[key][0])

    # Append std of sequential baseline to backend_perf_dict
    for backend_name in backends_std_dict:
        for key in backends_std_dict[backend_name].keys():
            backends_std_dict[backend_name][key].append(
                seq_baseline_std_dict[key][0])

    # Append speedup to backend_perf_dict
    for backend_name in backends_dict:
        for key in backends_dict[backend_name].keys():
            t_par = backends_dict[backend_name][key][0]
            t_seq = backends_dict[backend_name][key][1]
            speedup = t_seq / t_par
            backends_dict[backend_name][key].append(speedup)

    for backend_name in backends_std_dict:
        for key in backends_std_dict[backend_name].keys():
            t_par = backends_dict[backend_name][key][0]
            t_seq = backends_dict[backend_name][key][1]
            speedup = t_seq / t_par
            std_par = backends_std_dict[backend_name][key][0]
            std_seq = backends_std_dict[backend_name][key][1]
            std_speedup = ((std_seq/t_seq) + (std_par/t_par)) * speedup
            backends_std_dict[backend_name][key].append(std_speedup)

    return backends_dict, backends_std_dict

if __name__ == "__main__":

    parser = argparse.ArgumentParser()
    parser.add_argument("logs_path",
                        help="Path to the folder with run logs")
    parser.add_argument("-i", "--im_save_path",
                        help="path in which images will be saved")
    parser.add_argument("-p", "--pdf_save_path",
                        help="path in pdf will be saved")
    args = parser.parse_args()

    logs_path = args.logs_path
    im_save_path = args.im_save_path if args.im_save_path else ""
    pdf_save_path = args.pdf_save_path if args.pdf_save_path else ""

    pp = None
    if os.path.isdir(pdf_save_path):
        f_name = "mandelbrot_benchmark.pdf"
        pp = PdfPages(os.path.join(pdf_save_path, f_name))

    # =========================================================================
    #                      SWEEP OVER THE WORKLOAD SIZE
    # =========================================================================
    backend_names = ["hpx", "hpx_startstop", "tbb", "omp", "pthreads"]
    backends_size_dict, backends_size_devs_dict = extract_perf(backend_names,
                                                               Sweep.SIZE)


    fig = plot_backends_over_key(backend_names, backends_size_dict,
                                 backends_size_devs_dict, Metric.PARALLEL_TIME,
                                 title="Parallel processing time as function of "
                                 "image size",
                                 xlabel="Number of pixels",
                                 ylabel="Processing time [s]",
                                 save_path=im_save_path)
    if os.path.isdir(pdf_save_path):
        pp.savefig(fig)
    plt.show()

    fig = plot_backends_over_key(backend_names, backends_size_dict,
                                 backends_size_devs_dict, Metric.SEQUENTIAL_TIME,
                                 title="Sequential processing time as function of "
                                 "image size",
                                 xlabel="Number of pixels",
                                 ylabel="Processing time [s]",
                                 save_path=im_save_path)
    if os.path.isdir(pdf_save_path):
        pp.savefig(fig)
    plt.show()

    fig = plot_backends_over_key(backend_names, backends_size_dict,
                                 backends_size_devs_dict, Metric.SPEEDUP,
                                 title="Speed-up as function of "
                                 "image size",
                                 xlabel="Number of pixels",
                                 ylabel="Speed-up",
                                 save_path=im_save_path)
    if os.path.isdir(pdf_save_path):
        pp.savefig(fig)
    plt.show()

    # =========================================================================
    #               SWEEP OVER THE NUMBER OF Processing Units (PUs)
    # =========================================================================

    backend_names = ["hpx", "hpx_startstop", "tbb", "omp", "pthreads"]
    backends_pus_dict, backends_pus_std_dict = extract_perf(backend_names,
                                                            Sweep.NUM_PUS)

    fig = plot_backends_over_key(backend_names, backends_pus_dict,
                                 backends_pus_std_dict, Metric.PARALLEL_TIME,
                                 title="Parallel processing time as function of "
                                 "number of threads",
                                 xlabel="Number of threads",
                                 ylabel="Processing time [s]",
                                 save_path=im_save_path)
    if os.path.isdir(pdf_save_path):
        pp.savefig(fig)
    plt.show()

    fig = plot_backends_over_key(backend_names, backends_pus_dict,
                                 backends_pus_std_dict, Metric.SEQUENTIAL_TIME,
                                 title="Sequential processing time as function of "
                                 "number of threads",
                                 xlabel="Number of threads",
                                 ylabel="Processing time [s]",
                                 save_path=im_save_path)
    if os.path.isdir(pdf_save_path):
        pp.savefig(fig)
    plt.show()

    fig = plot_backends_over_key(backend_names, backends_pus_dict,
                                 backends_pus_std_dict, Metric.SPEEDUP,
                                 title="Speed-up as function of "
                                 "number of threads",
                                 xlabel="Number of threads",
                                 ylabel="Speed-up",
                                 save_path=im_save_path)
    if os.path.isdir(pdf_save_path):
        pp.savefig(fig)
    plt.show()


    # =========================================================================
    #                          SWEEP OVER THE NSTRIPES
    # =========================================================================
    backend_names = ["hpx", "hpx_startstop"]
    backends_nstripes_dict, backends_nstripes_stds_dict =\
        extract_perf(backend_names, Sweep.NSTRIPES,
                     use_sequential_baseline=False)
    fig = plot_backends_over_key(backend_names, backends_nstripes_dict,
                                 backends_nstripes_stds_dict,
                                 Metric.PARALLEL_TIME,
                                 title="Parallel processing time as function of "
                                 "nstripes",
                                 xlabel="nstripes",
                                 ylabel="Processing time [s]",
                                 save_path=im_save_path)
    if os.path.isdir(pdf_save_path):
        pp.savefig(fig)
    plt.show()

    backend_names = ["tbb", "omp", "pthreads"]
    backends_nstripes_dict, backends_nstripes_stds_dict =\
        extract_perf(backend_names, Sweep.NSTRIPES,
                     use_sequential_baseline=False)
    fig = plot_backends_over_key(backend_names, backends_nstripes_dict,
                                 backends_nstripes_stds_dict,
                                 Metric.PARALLEL_TIME,
                                 title="Parallel processing time as function of "
                                 "nstripes (other backends)",
                                 xlabel="nstripes",
                                 ylabel="Processing time [s]",
                                 save_path=im_save_path)
    if os.path.isdir(pdf_save_path):
        pp.savefig(fig)
    plt.show()

    # =========================================================================
    #                          Close the PDF
    # =========================================================================
    if os.path.isdir(pdf_save_path):
        pp.close()

