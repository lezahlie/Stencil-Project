import os
import sys
import subprocess

"""
    @file analyze-data.py
    @authors James Simmons, Leslie Horace
    @brief script to automate data gathering, analysis, and plot creation
    @version 1.0 
"""

def terminate(logFile):
    logFile.write(f'Usage: python3 gather-data.py <"mpi"|"pthread"> OPTIONAL: <size> <#process/threads> <iterations>\n')
    logFile.write("1. optional args <size> and <#process/threads> can be comma delimited lists with no spaces, e.g., 1,2,4\n")
    logFile.write("2. generate_matrix.py must be ran prior to gather_data.py with the same matrix sizes\nNote: sizes can be entered all at once or a few at a time\n")
    logFile.close()
    exit(1)


# parse program output, save in log, return times
def get_lines(result, cmd, df, lf):
    overallTime = 0
    computeTime = 0
    df.write(" ".join(cmd))
    output_lines = result.stdout.decode()
    read_lines = output_lines.split("\n")
    for line in read_lines:
        if 'Overall' in line:
            overallTime = float(line.split()[-2])            
        elif 'Compute' in line:
            computeTime = float(line.split()[-2])
        elif 'Error' in line:
            lf.writelines(output_lines)
            exit(1)
    df.writelines(output_lines)
    return overallTime, computeTime


# runs stencil programs and gathers timing data
def run_program(progType, size, proc_cnt, num_iter, dFile, lFile, d_dir):
    stencil_times = {
        'overall': {str(s): [] for s in size},
        'compute': {str(s): [] for s in size}
    }
    for s in size:
        mat_in = f'mat-{str(s)}.dat'
        for p in proc_cnt:
            mat_out = f'{d_dir}{progType}-{str(s)}-{str(p)}.dat'
            gcc_cmd = (progType == "mpi"
                and ['mpirun', '-np', str(p), './mpi-stencil-2d', str(num_iter), mat_in , mat_out, '0']
                or  ['./pth-stencil-2d', str(num_iter), mat_in, mat_out , '0', str(p)])
            result = subprocess.run((gcc_cmd), stdout=subprocess.PIPE, shell=False)
            overall, compute = get_lines(result, gcc_cmd, dFile, lFile)
            stencil_times['overall'][str(s)].append(overall)
            stencil_times['compute'][str(s)].append(compute)


def main():
    log_dir = "./logs/"
    if not os.path.exists(log_dir):
        os.makedirs(log_dir)
    script_log_path = f"{log_dir}gather_data_output.log"
    if not os.path.isfile(script_log_path):
        logFile = open(script_log_path, 'w')
    else:
        logFile = open(script_log_path, 'a')
    logFile.write(f"\n{'-'*60}\n")
    if (len(sys.argv) > 5 or len(sys.argv) < 2) or not (sys.argv[1] == "mpi" or sys.argv[1] == "pthread"):
        terminate(logFile)

    tmp = sys.argv[1].lower()
    prog_prefix = tmp.lower()
    if len(sys.argv) == 5:
        mat_size = [int(x) for x in sys.argv[2].split(',')]
        proc_count = [int(x) for x in sys.argv[3].split(',')]
        iteration = int(sys.argv[4])
    elif len(sys.argv) == 2:
        mat_size = [250, 500, 1000]
        proc_count = [1, 2, 4]
        iteration = 500

    data_dir = "./data/"
    if not os.path.exists(data_dir):
        os.makedirs(data_dir)

    stencil_data_path = f"{data_dir}{prog_prefix}_stencil_data.txt"
    if not os.path.isfile(stencil_data_path):
        dataFile = open(stencil_data_path, 'w')
    else:
        dataFile = open(stencil_data_path, 'a')

    logFile.write(f"\n> gathering {prog_prefix} stencil program timing data...")
    run_program(prog_prefix, mat_size, proc_count, iteration, dataFile, logFile, data_dir)
    logFile.write(f"\n# logged {prog_prefix} output in '{stencil_data_path}'") 

    dataFile.close()
    logFile.close()
    exit(0)
    
main()