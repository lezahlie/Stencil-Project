"""
    @file analyze-data.py
    @authors James Simmons, Leslie Horace
    @brief script to automate data gathering and debugging
    @version 1.0 
"""
import os
import subprocess

# print the standard output of the C program
def get_lines(result, cmd, diffs, file):
    output_lines = result.stdout.decode().split(f"\n{'-'*60}\n")
    for line in output_lines:
        if 'Error' in line:
            exit(1)
        elif "diff" in cmd:
            if 'differ' in line: 
                
                diffs+=1
            else: 
                print("No differences!", file=file)
        print(line, file=file)

# run pth-stencil-2d, mpi-stencil-2d
def run_programs(iteration, size, p_count, file):
    diffs =0
    for s in size:
        result = subprocess.run(['./make-2d', str(s), str(s), 'mat-'+str(s)+'.dat'], stdout=subprocess.PIPE)
        out_serial = 'serial-'+str(s)+'.dat'
        gcc_cmd = ['./stencil-2d', str(iteration), 'mat-'+str(s)+'.dat', out_serial]
        result = subprocess.run(gcc_cmd, stdout=subprocess.PIPE)
        print(" ".join(gcc_cmd), file=file)
        get_lines(result, gcc_cmd, diffs, file)
        for t in p_count:
            out_pth = 'pth-'+str(s)+'-'+str(t)+'.dat'
            gcc_cmd = ['./pth-stencil-2d', str(iteration), 'mat-'+str(s)+'.dat', out_pth, '1', str(t)]
            result = subprocess.run(gcc_cmd, stdout=subprocess.PIPE)
            print("".join(gcc_cmd), file=file)
            get_lines(result, gcc_cmd, diffs, file)
            diff_cmd = ['diff', 'serial-'+str(s)+'.dat',  out_pth]
            result = subprocess.run(diff_cmd, stdout=subprocess.PIPE)
            print(" ".join(diff_cmd), file=file)
            get_lines(result, diff_cmd, diffs, file)
        for p in p_count:
            out_mpi = 'mpi-'+str(s)+'-'+str(p)+'.dat'
            gcc_cmd = ['mpirun', '-np', str(p), './mpi-stencil-2d', str(iteration), 'mat-'+str(s)+'.dat' ,  out_mpi, '1']
            result = subprocess.run(gcc_cmd, stdout=subprocess.PIPE)
            print("".join(gcc_cmd), file=file)
            get_lines(result, gcc_cmd,  diffs, file)
            diff_cmd = ['diff', 'pth-'+str(s)+'-'+str(p)+'.dat', out_mpi]
            result = subprocess.run(diff_cmd, stdout=subprocess.PIPE)
            print(" ".join(diff_cmd), file=file)
            get_lines(result, diff_cmd, diffs, file)
    return diffs

def main():
    size = [100, 200, 500]
    p_count = [1, 2, 3]
    iteration = 200
    diffs = []

    log_dir = "./logs/"
    if not os.path.exists(log_dir):
        os.makedirs(log_dir)
    outfile = f'{log_dir}debug_data_output.log'
    if os.path.isfile(outfile):
        subprocess.call(['rm', outfile])

    subprocess.call(['make', 'clean', 'all'])

    with open(outfile, 'w') as file:
        print('\nrunning stencil programs and saving output in "'+outfile+'"....')
        diffs = run_programs(iteration, size, p_count, file)
        print(f"total files with differences = {diffs}")
    print('removing test data files....')
    subprocess.call(['make', 'delete-data'])

main()