#!/bin/bash
#SBATCH --job-name="gather_data"
#SBATCH --output="gather_data.%j.%N.out"
#SBATCH --account=#
#SBATCH --partition=#
#SBATCH --nodes=1 
#SBATCH -t 00:05:00 
#SBATCH --export=ALL 

module purge

module load shared cpu/0.15.4 slurm/expanse/21.08.8 sdsc/1.0 DefaultModules gcc/9.2.0 openmpi/4.1.1

module list

/usr/bin/python3 gather-data.py $1 $2 $3 $4
