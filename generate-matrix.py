"""
    @file generate-data.py
    @authors James Simmons, Leslie Horace
    @brief script to automate data gathering, analysis, and plot creation
    @version 1.0 
"""

import os
import sys
import subprocess

def main():
    if len(sys.argv) == 2:
        mat_size = [int(x) for x in sys.argv[1].split(',')]
    else:
        mat_size = [250, 500, 750, 1000]

    log_dir = './logs/'
    if not os.path.exists(log_dir):
        os.makedirs(log_dir)

    script_log_path = log_dir+"generate_matrix_output.log"
    logFile = open(script_log_path, 'w')
    
    for s in mat_size:
        make_data = subprocess.run(['./make-2d', str(s), str(s), f'mat-{str(s)}.dat'], stdout=subprocess.PIPE)
        sub_output = make_data.stdout.decode()
        logFile.writelines(sub_output)

    logFile.close()

    
main()