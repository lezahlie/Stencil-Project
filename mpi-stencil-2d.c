/**
 * @file mpi-stencil-2d.c
 * @authors Leslie Horace, James Simmons
 * @brief OpenMPI implentation of 9-pt stencil algorithm
 * @version 2.0
 * 
 */

#include "mpi_utils.h"
#include "utilities.h"

int mpiStencilLoop(ProcessData * pd, MatrixPointer * mp, StencilData * sd, ConditionBools cb, char * stacked_file){
    int ret = 0, next_a = 0, next_b = 0, a1 = 0, a2 = 0, b1 = 0, b2 = 0, w_count = 0, m_count = MATRIX_COUNT(pd->rows, pd->cols);
    double start_compute = 0.0, end_compute = 0.0;
    FILE * fp = NULL;

    // open all stacked file for writing and write initial matrix state
    if(cb.is_root && cb.write_state){
        if ((fp = fopen(stacked_file, "wb")) == NULL){
            printf("Error: cannot open/write to '%s'\n", stacked_file);
            goto stop_all; 
        }
        w_count = fwrite(mp->A, DOUBLE_SIZE, m_count, fp);
        if(handleIOError(fp, w_count, m_count, "[mpiStencilLoop:fwrite()]") == ERROR) goto stop_write;
    }
    if(cb.is_root && cb.print_state) print2D(mp->A, pd->rows, pd->cols);

    // perform stencil iterations
    for(int k = 0; k < sd->iterations; k++){
        start_compute=MPI_Wtime();
        for(int i= 1; i < pd->block_size-1; i++){
            stencil2D(mp->B, mp->C, i, pd->cols); 
        }
        // sum compute time for each process
        end_compute=MPI_Wtime()-start_compute;
        sd->compute_time+=end_compute;

        if(cb.is_parallel){
                // set up border exchange locations
                a1 = (IS_EVEN(pd->rank)) ? BOT_SOURCE(pd->block_size, pd->cols) : TOP_SOURCE(pd->cols);
                a2 = (IS_EVEN(pd->rank)) ? BOT_TARGET(pd->block_size, pd->cols) : TOP_TARGET;
                b1 = (IS_EVEN(pd->rank)) ? TOP_SOURCE(pd->cols) : BOT_SOURCE(pd->block_size, pd->cols); 
                b2 = (IS_EVEN(pd->rank)) ? TOP_TARGET : BOT_TARGET(pd->block_size, pd->cols);
                // find up neighboring ranks for each process
                next_a = (IS_EVEN(pd->rank) ? RIGHT(pd->rank, cb.is_root) : LEFT(pd->rank));
                next_b = (IS_EVEN(pd->rank) ? LEFT(pd->rank) : RIGHT(pd->rank, cb.is_root));

                // even exchange right, odd exchange left
                ret = MPI_Sendrecv(&mp->B[a1], pd->cols, MPI_DOUBLE, next_a, 99, 
                                    &mp->B[a2], pd->cols, MPI_DOUBLE, next_a, 99, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                if(handleMpiError(pd->rank, ret, "[MPI_Sendrecv(a)]") == ERROR) goto stop_write;

                // even exchange left, odd exchange right
                ret = MPI_Sendrecv(&mp->B[b1], pd->cols, MPI_DOUBLE, next_b, 99, 
                                    &mp->B[b2], pd->cols, MPI_DOUBLE, next_b, 99, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                if(handleMpiError(pd->rank, ret, "[MPI_Sendrecv(b)]") == ERROR) goto stop_write; 

            // gather if printing state or writing to all stacked file
            if(cb.print_state || cb.write_state){
                ret = MPI_Gatherv(&mp->B[pd->cols], MATRIX_COUNT(pd->block_size-2,pd->cols), MPI_DOUBLE, 
                                    &mp->A[pd->cols], mp->sub_count, mp->sub_offset, MPI_DOUBLE, pd->num_p-1, MPI_COMM_WORLD);
                if(handleMpiError(pd->rank, ret, "[mpi-stencil-2d:mpiStencilLoop:MPI_Gatherv()]") == ERROR) goto stop_write;
            }
        }

        // write each matrix state to all file
        if(cb.is_root && cb.write_state){
            w_count = fwrite((cb.is_parallel) ? (mp->A) : (mp->B), DOUBLE_SIZE, m_count, fp);
            if(handleIOError(fp, w_count, m_count, "[mpiStencilLoop:fwrite()]") == ERROR) goto stop_write;
        }

        // print each matrix state if debug level = 2
        if(cb.is_root && cb.print_state) print2D(mp->A, pd->rows, pd->cols);
        // swap sub matrix pointers
        swap2D(&mp->B, &mp->C); 
    } 

    ret = SUCCESS;

stop_write:
    if(cb.is_root && cb.write_state) fclose(fp);
stop_all:
    return ret;
}


int main (int argc, char **argv) {
    double start_overall = 0, end_overall = 0, max_compute = 0;
    int ret = EXIT_FAILURE;
    // local process structs 
    ProcessData pd = {0, 0, 0, 0, 0};
    StencilData sd = {0, 0, 0.0};
    MatrixPointer mp = {NULL, NULL, NULL, NULL, NULL};

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &pd.rank);
    MPI_Comm_size(MPI_COMM_WORLD, &pd.num_p);
    // set up error handler to return error msgs before aborting
    MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);
    

    FileData fd = {argv[2], argv[3], (argc == 6) ? argv[5] : NULL};
    ConditionBools cb = {EQUAL(pd.num_p-1, pd.rank), NOT_EQUAL(pd.num_p, 1), NOT_EQUAL(fd.allfile, NULL), 0, 0};

    if(cb.is_root) start_overall = MPI_Wtime();

    if(argc < 5 || argc > 6){
        if(!pd.rank){
            printf("Usage: mpirun -np <num processes> %s <num_iterations> <infile> <outfile> <debug_level[0-2]> <all_stacked_file(optional)>\n", argv[0]);
            FLUSH_OUTPUT
        } terminate(ret);
    }

    
    // parse initial data and read in matrix
    if(cb.is_root){
        if((sd.debug_level = parseInt(argv[4],0,2,"debug_level[0-2]")) == ERROR) abortComm(pd.rank, NULL, ret);
        if((sd.iterations = parseInt(argv[1],1,SKIP_ARG,"num_iterations")) == ERROR) abortComm(pd.rank, NULL, ret);
        if(read2D(&mp.A, &pd.rows, &pd.cols, fd.initfile) == ERROR) abortComm(pd.rank, NULL, ret);
        if(pd.num_p > pd.rows-2) abortComm(pd.rank, "[mpi-stencil-2d:main]: <num_processes> > block_size", ret);
        if(malloc1D((void*)&mp.sub_offset, pd.num_p*INT_SIZE, "sub_offset") == ERROR)  goto clean_a;
        if(malloc1D((void*)&mp.sub_count, pd.num_p*INT_SIZE, "sub_count") == ERROR) goto clean_b;
    }


    if(cb.is_parallel){
        // set all data for scattering and share static data with all p
        setScatterData(&pd, &mp, &sd, cb);
        // malloc sub matrix for stencil loop
        if(malloc1D((void*)&mp.B, MATRIX_SIZE(pd.block_size,pd.cols),"mp.B") == ERROR) goto clean_c;
        // scatter parent matrix to all sub matrices
        ret = MPI_Scatterv(mp.A, mp.sub_count, mp.sub_offset, MPI_DOUBLE, mp.B, MATRIX_COUNT(pd.block_size, pd.cols), MPI_DOUBLE, pd.num_p-1, MPI_COMM_WORLD);
        if(handleMpiError(pd.rank, ret, "mpi-stencil-2d:main:MPI_Scatterv()") == ERROR) goto clean_all;
    }else {
        pd.block_size = pd.rows;  
        mp.B = mp.A;
    }
    
    // check is debugging is turned on
    cb.print_state = EQUAL(sd.debug_level, 2);
    cb.debug_on = NOT_EQUAL(sd.debug_level, 0);

    // copy and initialize matrix for stencil computations 
    if(malloc1D((void*)&mp.C, MATRIX_SIZE(pd.block_size,pd.cols),"mp.C") == ERROR) goto clean_d;
    init2D(mp.C, pd.block_size, pd.cols); 

    // perform stencil iterations 
    if(cb.is_root && cb.debug_on) printf("Running %d stencil iterations with %d processes...\n", sd.iterations, pd.num_p);
    if(mpiStencilLoop(&pd, &mp, &sd, cb, fd.allfile) == ERROR) goto clean_all;

    if(cb.is_parallel){
        // gather all sub matrices into parent matrix
        ret = MPI_Gatherv(&mp.C[pd.cols], MATRIX_COUNT(pd.block_size-2,pd.cols), MPI_DOUBLE, &mp.A[pd.cols], mp.sub_count, mp.sub_offset, MPI_DOUBLE, pd.num_p-1, MPI_COMM_WORLD);
        if(handleMpiError(pd.rank, ret, "mpi-stencil-2d:main:MPI_Gatherv()") == ERROR) goto clean_all;
        // process with the maximum compute is the overall compute time
        ret = MPI_Reduce(&sd.compute_time, &max_compute, 1, MPI_DOUBLE, MPI_MAX, pd.num_p-1, MPI_COMM_WORLD);
        if(handleMpiError(pd.rank, ret, "mpi-stencil-2d:mpiStencilLoop:MPI_Reduce()") == ERROR) abortComm(pd.rank, NULL, ret);
    }else  max_compute = sd.compute_time;  

    // write out the final matrix state, file info, and timing analysis
    if(cb.is_root){
        if(write2D((pd.num_p == 1) ? (mp.C) : (mp.A), pd.rows, pd.cols, fd.finalfile) == ERROR) goto clean_all;
        end_overall = MPI_Wtime();
        if(cb.debug_on){
            printDataFileInfo(fd.finalfile, pd.rows, pd.cols, 0);
            if(fd.allfile != NULL) printStackedFileInfo(fd.allfile, pd.rows, pd.cols, sd.iterations);
            FLUSH_OUTPUT
        }
        printTimes(end_overall-start_overall, max_compute);
        FLUSH_OUTPUT
    }

    ret = EXIT_SUCCESS;

clean_all:
    free(mp.C);
clean_d:
    free(mp.B);
clean_c:
    if(cb.is_root) free(mp.sub_count);
clean_b:
    if(cb.is_root) free(mp.sub_offset);
clean_a:
    if(cb.is_parallel) free(mp.A);
    terminate(ret);
}
