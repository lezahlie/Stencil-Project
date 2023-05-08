#!/bin/bash
#SBATCH --job-name="generate_matrix"
#SBATCH --output="generate_matrix.%j.%N.out"
#SBATCH --partition=shared
#SBATCH --account=ccu107
#SBATCH --mem=128GB
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=1
#SBATCH --cpus-per-task=1
#SBATCH -t 00:05:00
#SBATCH --export=ALL

module purge
module load shared cpu/0.15.4 slurm/expanse/21.08.8 sdsc/1.0 DefaultModules gcc/9.2.0 openmpi/4.1.1
module list

# compile gcc and mpicc programs
make clean all
# clearout old data
make delete-data

/usr/bin/python3 ./generate-matrix.py $1