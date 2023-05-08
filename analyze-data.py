"""
    @file analyze-data.py
    @authors James Simmons, Leslie Horace
    @brief script to automate data gathering, analysis, and plot creation
    @version 1.0 
"""

import os
import sys
import subprocess
import seaborn as sns
import matplotlib.pyplot as plt


def parse_cmd(chunk):
    tmp_cmd = chunk[0].split(' ')
    if "mpirun" in chunk[0]:
        proc = tmp_cmd[2]
        mat_str = tmp_cmd[5]
        size = mat_str[4:-4]
    else:
        proc = tmp_cmd[-1]
        mat_str = tmp_cmd[-4]
        size = int(mat_str[4:-4])
    p = int(proc)
    s = int(size)
    return p, s


# parse program output, save in log, return times
def get_times(section, procs, times):
    lines = section.split('\n')
    proc, size = parse_cmd(lines)
    overallTime = float(lines[1].split(' ')[-2])
    k = procs.index(proc)
    times['overall'][size][k][1] = overallTime
    computeTime = float(lines[3].split(' ')[-2])
    times['compute'][size][k][1] = computeTime
    return times


# runs stencil programs and gathers timing data
def read_data(inFile):
    sizes = []
    procs = []
    sections = inFile.read().split(f"\n{'-'*60}\n")

    for sec in sections[0:-2]:
        chunk = sec.split('\n')
        proc, size = parse_cmd(chunk)
        if size not in sizes:
            sizes.append(size)
        if proc not in procs:
            procs.append(proc)
    times = {
        'overall': {s: [[p, 0.0] for p in procs] for s in sizes},
        'compute': {s: [[p, 0.0] for p in procs] for s in sizes}
    }
    j=0
    for s in sizes:
        for k in range(len(procs)):
            times = get_times(sections[j], procs, times)
            j+=1
    inFile.close()
    return sizes, procs, times


# calculates and returns speedup data set
def calculate_speedup(time_data, procs, size):
    speedup_data = {
        'overall': {s: [[p, 0.0] for p in procs] for s in size},
        'compute': {s: [[p, 0.0] for p in procs] for s in size}
    }
    for key in speedup_data.keys():
        for s in size:
            serial_time = time_data[key][s][0][1]
            speedup_data[key][s][0][1] = 1.0
            for k in range(1, len(procs)):
                parallel_time = time_data[key][s][k][1]
                speedup_data[key][s][k][1] = (serial_time/parallel_time)
    return speedup_data


# calculates and returns efficiency data set
def calculate_efficiency(speedup_data, procs, size):
    efficiency_data = {
        'overall': {s: [[p, 0.0] for p in procs] for s in size},
        'compute': {s: [[p, 0.0] for p in procs] for s in size}
    }
    eff_max=1.05
    for key in efficiency_data.keys():
        for s in size:
            efficiency_data[key][s][0][1] = 1.0
            for k in range(1, len(procs)):
                speedup = speedup_data[key][s][k][1]
                efficiency_data[key][s][k][1] = (speedup / procs[k])
                if efficiency_data[key][s][k][1]>eff_max:
                    eff_max = efficiency_data[key][s][k][1]
    return efficiency_data, eff_max


# calculates and returns karp flatt metric data set
def calculate_karp_flatt(speedup_data, procs, size):
    karp_flatt_data = {
        'overall': {s: [[p, 0.0] for p in procs] for s in size},
        'compute': {s: [[p, 0.0] for p in procs] for s in size}
    }
    karp_min=0
    for key in karp_flatt_data.keys():
        for s in size:
            for k in range(1, len(procs)):
                speedup = speedup_data[key][s][k][1]
                e_metric = (((1 / speedup) - (1 / procs[k]))/(1 - (1 / procs[k])))
                karp_flatt_data[key][s][k][1] = e_metric
                if e_metric<karp_min:
                    karp_min = e_metric
    return karp_flatt_data, karp_min


# generates overall and computation plots for each plot types
def create_plots(plot_data, outlier, sizes, procs, plotPath, plotType, yTitle):
    sns.set_style("whitegrid")
    # Adjust the figsize and the space between plots
    fig, axes = plt.subplots(ncols=2, figsize=(18, 6))
    plot_subtype = ["Overall", "Computation"]
    compType = "mpi" in plotPath and "OpenMPI" or "Pthreads"

    data_set = [
        {s: [plot_data['overall'][s][k][1] for k in range(len(procs))] for s in sizes},
        {s: [plot_data['compute'][s][k][1] for k in range(len(procs))] for s in sizes}
    ]

    for t, ax in enumerate(axes):
        for s in sizes:
            sns.lineplot(x=procs, y=data_set[t][s], ax=ax, label=f'{s}x{s}', marker='o')   
            if plotType == "Time":
                outlier = max(data_set[t][s])
        if plotType == "Speedup":
            ax.set_ylim(1, procs[-1])
            sns.lineplot(x=procs, y=procs, ax=ax, label=f'Ideal', linestyle="--")
        elif plotType == "Efficiency": 
            ax.set_ylim(0, (outlier+0.01))
            sns.lineplot(x=procs, y=1, ax=ax, label=f'Ideal', linestyle="--")
        elif plotType == "e":
            ax.set_ylim((outlier-0.01), 1.01)
            sns.lineplot(x=procs, y=0, ax=ax, label=f'Ideal', linestyle="--")
        else:
            ax.set_ylim(0, (outlier+5))
        ax.set_ylabel(yTitle)
        ax.set_xlabel("mpi" in plotPath and "#p (processes)" or "#t (threads)" )
        ax.set_xticks(range(procs[0],procs[-1]+1))
        ax.set_xlim(procs[0],procs[-1])
        ax.set_title(f"{compType}: {plotType}_{plot_subtype[t]}")
        ax.minorticks_on()
        ax.grid(True, which='minor', color='gray', linewidth=0.1)
        ax.legend(loc='upper left', bbox_to_anchor=(1, 1), ncol=1)

    plt.tight_layout()
    fname = f'{plotPath}-{plotType}.png'
    plt.savefig(fname, dpi=600)


def main():
    log_dir = "./logs/"
    if not os.path.exists(log_dir):
        os.makedirs(log_dir)
    script_log_path = f"{log_dir}analyze_data_output.log"
    logFile = open(script_log_path, 'w')

    data_dir = "./data/"
    tmp = sys.argv[1]
    prog_prefix = tmp.lower()
    if len(sys.argv)==2 and (prog_prefix == 'pthread' or sys.argv[1] == 'mpi'):
        data_path = f'{data_dir}{prog_prefix}_stencil_data.txt'
    else:
        logFile.write(f'Usage: python3 {sys.argv[0]} <mpi | pthread>')
        logFile.write("\nNote: files generated from scripts 'generate-matrix.py' and 'gather-data.py' are prerequisites")
        exit(1)


    plot_dir = "./plots/"
    if not os.path.exists(plot_dir):
        os.makedirs(plot_dir)
    else:
        subprocess.run((['rm', '-r', '-f', plot_dir+"*.png"]), stdout=subprocess.PIPE)

    plot_path_prefix = f'{plot_dir}{prog_prefix}'

    if not os.path.isfile(data_path):
        logFile.write(f"Error [open]: '{data_path}' does not exist\nNote: 'generate-matrix.py' => 'gather-data.py' are prerequisites scripts ")
        exit(1)
    
    dataFile = open(data_path, 'r')
    logFile.write(f"> reading in time data from '{data_path}'...\n")
    mat_size, node_count, times = read_data(dataFile)

    plot_type = ["Time", "Speedup", "Efficiency", "e"]
    y_title = ["Time (sec)","Sp (x's)", "Eff (%)", "e (%)"]

    logFile.write(f"\n> calculating {prog_prefix} overall and computation speedup...")
    speedup = calculate_speedup(times, node_count, mat_size)

    logFile.write(f"\n> calculating {prog_prefix} overall and computation efficiency...")
    efficiency, eff_max = calculate_efficiency(speedup, node_count, mat_size)

    logFile.write(f"\n> calculating {prog_prefix} overall and computation karp_flatt metric...")
    karp_flatt, karp_min = calculate_karp_flatt(speedup, node_count, mat_size)

    logFile.write(f"\n> generating {prog_prefix} time plots...")
    create_plots(times, 0, mat_size, node_count, plot_path_prefix, plot_type[0], y_title[0])
    
    logFile.write(f"\n> generating {prog_prefix} speedup plots...")
    create_plots(speedup, 0, mat_size, node_count, plot_path_prefix , plot_type[1], y_title[1])
    
    logFile.write(f"\n> generating {prog_prefix} efficiency plots...")
    create_plots(efficiency, eff_max, mat_size, node_count, plot_path_prefix , plot_type[2], y_title[2])
    
    logFile.write(f"\n> generating {prog_prefix} karp flatt metric plots...")
    create_plots(karp_flatt, karp_min, mat_size, node_count, plot_path_prefix, plot_type[3], y_title[3])

    logFile.write(f"\n# saved all plots in directory '{plot_dir}'")

    logFile.close()

    
main()