```
Authors: Leslie Horace, James Simmons
Project: Stencil-Project
Purpose: Data Collection and Analysis for Parallel applications of 9-pt Stencil Iterations
Note: It is reccommended to open this readme with a markdown viewer
```


# Prequsite Installations (Ubuntu-Debian)
*Note: Documentation links included for other environment installations*
> Update OS Packages First!
>- `sudo apt update` 
>
>[OpenMPI](https://docs.open-mpi.org/en/v5.0.x/installing-open-mpi/quickstart.html)
>- `sudo apt install openmpi-bin openmpi-dev openmpi-common openmpi-doc libopenmpi-dev`
>
>[Python3 [min-version 3.5]](https://realpython.com/installing-python)
>- `sudo apt install python3`
>
>[Pip3 [optional]](https://matplotlib.org/stable/users/installing/index.html)
>- `sudo apt install python3-pip`
>
>[Matplotlib](https://matplotlib.org/stable/users/installing/index.html)
>
>- `pip3 install matplotlib`
>
>[Seaborn](https://seaborn.pydata.org/installing.html)
>- `pip3 install seaborn`
>
>[Numpy](https://numpy.org/install/)
>
>- `pip3 install numpy`
>
>[Opencv](https://pypi.org/project/opencv-python/) 
>- `pip3 install opencv-python`



# Project Files Overview

### GCC Programs 

---

*Note: `<arg>` is an input, `[arg]` is optional, `arg1 | arg2` means arg1 or arg2.*

1. Makefile
- `Usage: make [clean] [all | <program>]`
- Compiles/cleans all project programs 
2. make-2d.c
-   `Usage: ./make-2d <rows> <cols> <outfile>`
- Generates a matrix and initializes values to represent a boilerplate
3. print-2d.c
- `Usage: ./print-2d <infile>`
- Prints a matrix to console (debug_level=2)
4. stencil-2d.c
- `Usage: ./stencil-2d <num_iterations> <infile> <outfile> <all_stacked_file(optional)>` 
- Serial version of 9-pt stencil algorithm 
5. pth-stencil-2d.c
- `Usage: ./pth-stencil-2d <num_iterations> <infile> <outfile> <debug_level[0-2]> <num_threads> <all_stacked_file(optional)>`
- Pthread version of 9-pt stencil algorithm 
6. mpi-stencil-2d.c
- `Usage: `
- OpenMPI version of 9-pt stencil algorithm

### GCC Utility & Header Files

---

1. utilities.c
- User defined functions shared utilized by entire project
2. utilities.h
- Header file containing structs, macros, and protoypes in "utilities.c"
3. mpi_utils.c
- User defined functions for "mpi-stencil-2d.c"
4. mpi_utils.h
- Header file containing structs, macros, and protoypes for "mpi-stencil-2d.c"
- "utilities.h" is linked here, giving access to all prototype functions
5. timer.h
- Gets the current time in microseconds
### Python Scripts 

---
*Note: All pythons scripts save output to log files in `./logs/` subfolder*
1. generate-matrix.py
- `Usage: python3 generate-matrix.py <#rows/cols>`
    - *`<#rows/cols>` can be comma delimited lists with no spaces, e.g., `"1,2,4"`*
-  Runs "make-2d" 
    - *Only creates square matrices*
    
2. gather-data.py
- `Usage: python3 gather-data.py <pthread/mpi> OPTIONAL: [size] [process/threads] [iterations]`
    - *If at least one optional arg is entered, all must be entered in the defined order*
    - *Optional args `[size]` and `[process/threads]` can be entered as a comma delimited string list with no spaces, e.g., `"1,2,4"`*
- "generate-matrix.py" must be ran prior to "gather-data.py" with the same matrix sizes
- Runs "pthread-stencil-2d" or "mpi-stencil-2d"

3. analyze-data.py
- `Usage: python3 analyze-data.py <pthread/mpi>`
- Parses time data files generated from "gather_data.py"
- Calculates speedup, efficiency, and serial fraction 
- Generates graphs for time and all calculated data

4. create-video.py 
- `Usage: python3 create-video.py <stackedfile>`
    - *`<stackedfile>` name must include in order, <#rows> <#cols> <#iterations> demimited by any non-numeric chars*
    - *e.g., `xyz-100.abc100.def25.raw-h3ll0` will parse `100` rows x `100` cols x `25` iterations, any #'s after <#iterations> is excluded*
- `(Alt) Usage: python3 <rows> <columns> <iterations>`
    - Utilizes the Makefile and C files to create a raw file, with the rows columns and iterations specified
    - Takes the stacked file generated and creates an mp4 video from it
- Creates a video from stacked raw file

5. debug-data.py
- `Usage: python3 debug-data.py`
- Runs "stencil-2d", "pthread-stencil-2d", and "mpi-stencil-2d"
- Diffs final matrix files for "stencil-2d" and "pthread-stencil-2d"
- Diffs "pthread-stencil-2d" and "mpi-stencil-2d" to cross check

### Slurm Job Scheduling Scripts 

---

1. submit-jobs.bash
- `Usage: bash ./submit-jobs.bash`
- Dynamically schedules all jobs
- Defines sbatch flags and input params
    - Schedules "sub-generate.bash"
    - Schedules "sub-gather.bash"
- Calls 
2. sub-generate.bash
- `Usage: sbatch ./sub-generate.bash <matrix-size | matrix-size-list>`
    - *Both `<matrix-size>` and `<matrix-size-list>` count as 1 string parameter*
- Job script to create input matrices
- Compiles the programs
- Runs "generate-matrix.py"
3. sub-gather.bash
- `Usage: sbatch --mem=<#GB> --cpus-per-task=1 --ntasks-per-node=<#tasks> ./sub_gather.bash <"mpi" | "pthread"> <#size> <#process> <#iterations>`
- Job script to run stencil programs
- runs "gather-data.py" for 


# Project Usage and How to Guides


## Part 1: Data Gathering

---

### [Local Machine]
#### Manual Method:
>#### 1. Compile programs with Makefile
> - Run: `make` | `make all` 
>#### 2. Create a matrix with make-2D
> - Run: `./make-2d <rows> <cols> <outfile>`
>#### 3. Run pthreads-stencil-2d
> - Run: `./pth-stencil-2d <num_iterations> <infile> <outfile> <debug_level[0-2]> <num_threads> <all_stacked_file(optional)>`
>#### 4. Run mpi-stencil-2d
> - Run: `mpirun -np <num processes> ./mpi-stencil-2d <num_iterations> <infile> <outfile> <debug_level[0-2]> <all_stacked_file(optional)>`
#### Script Method:
>#### 1. Compile programs with Makefile
> - Run: `make` | `make all` 
>#### 2. Create matrices with gather-matrix.py
> - Run: `./make-2d <rows> <cols> <outfile>`
> - Script will automatically generate output file names as `./mat-[size].dat`
> - All files are stored in project files directory
>#### 3. Run gather-data.py for pthreads
> - Run: `python3 gather-data.py pthread [size_string_list] [threads_list] [iterations]`
> - Script will create directory `./data` if it doesn't exist
> - Saves all output matrices with name `./data/pthread-[size]-[#p].dat` and captured results to `./data/pthread_stencil_data.txt`
>#### 4. Run gather-data.py for OpenMPI
> - Run: `python3 gather-data.py mpi [size_string_list] [threads_list] [iterations]`
> - Script will create directory `./data` if it doesn't exist
> - Saves all output matrices with name `./data/mpi-[size]-[#p].dat` and captured results to `./data/mpi_stencil_data.txt`
>
> *Note: Python Scripts only support square matrices, must use manual method to compute non square matrices*
> *Addionally gather-data.py does not support stacked raw file creation, use manual method or create-video.py*
---

### [SLURM Environments] 
>#### 1. Setup python script parameters and sbatch flags in "submit_all.bash"
>- set matrix size string list param (generate-matrix.py)
>   - `MAT="5000,10000,20000,40000"` 
>- set number of iterations param (gather-data.py)
>   - `NI=12`  
>- set proccess/threads list (gather-data.py)
>   - `procs=(1 2 4 8 16)`   
>- set matrix size list (gather-data.py)
>   - `sizes=(5000 10000 20000 40000)`  
>- set memory per size list (gather-data.py)
>   - `mems=(8GB 16GB 32GB 64GB)`
>  
> *Note: The values show in the examples above are already set in "submit_all.bash"*   
>#### 3. run submit-jobs.bash 
>- $ `bash ./submit-all.bash`
>
>*Note: This will submit individual jobs for both pthreads and mpi automatically*


## Part 2: Data Analysis 

---

### [Local Machine Only]
>#### 1. Pthreads Performance
> - Ensure `./data/pthread_stencil_data.txt` generated from "gather_data.py" exists
> - Run: `python3 ./analyze-data.py mpi`
> - The following graphs will save in `./plots/` subfolder
>   - pthread-Time.png
>   - pthread-Speedup.png
>   - pthread-Efficiency.png
>   - pthread-e.png
>#### 2. OpenMPI Performance
> - Ensure `./data/mpi_stencil_data.txt` generated from "gather_data.py" exists
> - Run: `python3 ./analyze-data.py pthread`
> - The following graphs will save in `./plots/` subfolder
>   - mpi-Time.png
>   - mpi-Speedup.png
>   - mpi-Efficiency.png
>   - mpi-e.png
>#### 2. Create mp4 video of all iterations
> - Run: `python3 ./create-video.py <stackedfile>`
> - Alt: `python3 <rows> <columns> <iterations>`
>   - Script will create a raw file named `[rows]-[cols]-[iterations].raw`
> - The generated video will save in `./videos/` subfolder
> - Video name is auto generated as `heatmap-[rows]x[cols]x[iterations].mp4`
> *Note: create-video.py uses pth-stencil-2d with 2 threads to generate raw files*
> *If the above is not preferred, please used the manual method to create raw file*
