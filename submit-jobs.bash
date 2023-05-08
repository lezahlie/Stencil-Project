#!/bin/bash

# generate_matrix.py takes a string list, so 1 parameter
MAT="5000,10000,20000,40000" 

# genrate all the matrices
sbatch ./sub-generate.bash $MAT

sleep 15  # sleep before scheduling next job

NI=12                               # num iterations
procs=(1 2 4 8 16)                  # num of cores/threads
sizes=(5000 10000 20000 40000)      # num rows/cols
mems=(8GB 16GB 32GB 64GB)           # mem sizes (set for each row/col size)

# create each jobs sequentially
for i in {0..4}; do
    NP=${procs[i]}
    for j in {0..3}; do
        S=${sizes[j]}
        M=${mems[j]}
        # run gather-data.py with pthreads
        sbatch --mem=$M --cpus-per-task=$NP --ntasks-per-node=1 ./sub-gather.bash pthread $S $NP $NI
        sleep 15 # sleep before scheduling next job
        # run gather-data.py with mpi
        sbatch --mem=$M --cpus-per-task=1 --ntasks-per-node=$NP ./sub-gather.bash mpi $S $NP $NI
        sleep 15 # sleep before scheduling next job
    done
done

# note: python script saves everything to the program directory, no need copy from scratch