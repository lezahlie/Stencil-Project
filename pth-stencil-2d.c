/**
 * @file pth-stencil-2d.c
 * @author Leslie Horace
 * @brief Main program for performing and recording 9-pt serial stencil operations
 * @version 2.0
 */
#include "utilities.h"

void * pthStencilLoop(void* tp_ptr){
    ThreadPrivate * tp = tp_ptr;
    double start_compute = 0.0, end_compute = 0.0;
    size_t w_count = 0, m_count = MATRIX_COUNT(tp->m_data->rows,tp->m_data->cols);
    double * A = tp->m_data->A, * B = tp->m_data->B;
    FILE * fp = NULL;
    int ret = 0;

    if(tp->rank == 0){
        // check if debugging is level 2 for printing matrix state
        if(tp->s_data->debug_level == 2) print2D(B, tp->m_data->rows, tp->m_data->cols);
        // check if writeRaw option is true for stacked raw file
        if(tp->f_data->allfile != NULL){
            // open the file for writing and check for errors
            if ((fp = fopen(tp->f_data->allfile, "wb")) == NULL){
                printf("Error: cannot open/write to '%s'\n", tp->f_data->allfile);
                goto stop_all;
            }
            // write to stacked raw file and check for errors
            w_count = fwrite(B, DOUBLE_SIZE, m_count, fp);
            if(handleIOError(fp, w_count, m_count, "[pthStencilLoop:fwrite()]") == ERROR) goto stop_write;
        }
    }

    // start blocked stencil iterations 
    for(int i = 0; i< tp->s_data->iterations; i++){
        GET_TIME(start_compute);  
        // perform blocked stencil algorithm
        for(int i = tp->block_start; i < (tp->block_start+tp->block_size); i++){
            stencil2D(A, B, i, tp->m_data->cols);  
        }
        GET_TIME(end_compute);  
        tp->thread_compute += (end_compute-start_compute);  

        // wait for all threads to finish this iteration andbreak if error
        ret = pthread_barrier_wait(&tp->t_shared->barrier);
        if(handleBarrier(ret, "Error [pth-stencil-2d:pthStencilLoop:pthread_barrier_wait()]") == ERROR) break; 
        if(tp->rank == 0){
            // print matrix state if debug level is 2
            if(tp->s_data->debug_level == 2) print2D(A, tp->m_data->rows, tp->m_data->cols);
            // write to stacked raw file if exists, stop if error
            if(tp->f_data->allfile != NULL){
                w_count = fwrite(A, DOUBLE_SIZE, m_count, fp);
                if(handleIOError(fp, w_count, m_count, "[pth-stencil-2d:pthStencilLoop:fwrite()]") == ERROR) goto stop_write;
            }
        }

        // swap matrix pointers for next iteration
        swap2D(&A, &B); 
    }

    // set local matrix ptrs back to shared matrix ptrs
    if(tp->rank == 0) {
        tp->m_data->A = A;
        tp->m_data->B = B;
    }
    // max thread time is the overall compute time
    if(MAX(tp->s_data->compute_time, tp->thread_compute) == tp->thread_compute) tp->s_data->compute_time = tp->thread_compute;

stop_write:
    if(tp->f_data->allfile != NULL && tp->rank == 0) fclose(fp);
stop_all:
    return NULL;
}

int main(int argc, char **argv) {
    int ret = EXIT_FAILURE; 
    double start_overall = 0.0, end_overall = 0.0;
    GET_TIME(start_overall);                                 

    // check if 6 or 7 args were entered
    if (argc < 6 || argc > 7){ 
        printf("Usage: %s <num_iterations> <infile> <outfile> <debug_level[0-2]> <num_threads> <all_stacked_file (optional)>\n", argv[0]);
        goto end_all;
    }

    // initialize structs shared between threads
    StencilData sd = {.iterations=0,.debug_level=0,.compute_time=0.0};
    ThreadShared ts = {.num_threads=0};
    FileData fd = {argv[2], argv[3], (argc == 7) ? argv[6] : NULL};
    MatrixData md = {NULL, NULL, 0, 0};

    // parse <num_iterations> <debug_level[0-2]> <num_threads> arguments, end if error
    if((sd.iterations = parseInt(argv[1], 1, SKIP_ARG, "num_iterations")) == ERROR) goto end_all;
    if((sd.debug_level = parseInt(argv[4], 0, 2, "debug_level")) == ERROR) goto end_all;
    if((ts.num_threads = parseInt(argv[5], 1, SKIP_ARG, "num_threads")) == ERROR) goto end_all;

    // read in matrix A from infile and check for errors
    if(read2D(&md.A, &md.rows, &md.cols, fd.initfile) == ERROR) goto end_all;
    // show warning if num_threads > blockable rows (users choice)
    if(MAX(ts.num_threads,md.rows-2) == ts.num_threads){
        printf("Warning [pth-stencil-2d:main]: num_threads[%d] > blockable rows[%d]\n", ts.num_threads, md.rows-2);
    }
    // malloc space for duplicate matrix B and check for errors
    if(malloc1D((void*)&md.B, MATRIX_SIZE(md.rows, md.cols), "md.B") == ERROR) goto end_a;

    // pointers for thread handles and thread private struct
    pthread_t * th_handles = NULL;
    ThreadPrivate * tp = NULL, * tp_tmp = NULL;

    // malloc thread_handles and private data struct, end if error
    if(malloc1D((void*)&th_handles, ts.num_threads*PTR_SIZE, "th_handles") == ERROR) goto end_b;
    if(malloc1D((void*)&tp, ts.num_threads*sizeof(ThreadPrivate), "tp")  == ERROR) goto end_c;
    tp_tmp = tp;    // save start addr 

    // initialize pthread barrier with number of threads and check for error
    ret = pthread_barrier_init(&ts.barrier, NULL, ts.num_threads);
    if(handleBarrier(ret, "Error [pth-stencil-2d:main:pthread_barrier_init()]") == ERROR) goto end_d;

    init2D(md.B, md.rows, md.cols);    // initialize matrix B
    // initialize/compute private thread data, then create threads
    if(sd.debug_level > 0) printf("Running %d stencil iterations with %d threads...\n", sd.iterations, ts.num_threads);
    
    for(int tid = 0; tid < ts.num_threads; tid++){
        tp->m_data = &md;
        tp->f_data = &fd;
        tp->s_data = &sd;
        tp->t_shared = &ts;
        tp->rank = tid;
        tp->block_start = BLOCK_LOW(tid, ts.num_threads, md.rows-2)+1;
        tp->block_size = BLOCK_SIZE(tid, ts.num_threads, md.rows-2);
        tp->thread_compute = 0.0;

        // create each thread, pass thread data struct, and void function, check for errors
        if((ret = pthread_create(&th_handles[tid], NULL, pthStencilLoop, (void*)tp)) != SUCCESS){
            errno = ret;
            perror("Error [pth-stencil-2d:main:pthread_create()]");
            ts.num_threads = tid+1; // set number of threads for joining only created threads
            break;  // break thread creation since we are missing threads
        } 
        tp++;
    }

    // join each thread to the parent thread and check for errors
    for (int tid = 0;  tid < ts.num_threads; tid++){
        if((ret = pthread_join(th_handles[tid], NULL)) != SUCCESS){
            errno = ret;
            perror("Error [pth-stencil-2d:main:pthread_join()]");
        }
    }

    // destroy the barrier and check for any errors
    ret = pthread_barrier_destroy(&ts.barrier);
    if(handleBarrier(ret, "Error [pth-stencil-2d:main:pthread_barrier_destroy()]") == ERROR) goto end_d;

    // write final matrix state to outfile
    if(write2D(md.B, md.rows, md.cols, fd.finalfile) == ERROR) goto end_d;
    if(sd.debug_level > 0){
        printDataFileInfo(fd.finalfile, md.rows, md.cols, 0);
        if(fd.allfile != NULL) printStackedFileInfo(fd.allfile, md.rows, md.cols, sd.iterations);
    }

    // calculate times and print
    GET_TIME(end_overall);
    printTimes((end_overall-start_overall), sd.compute_time);

    ret = EXIT_SUCCESS;

end_d: 
    tp = tp_tmp;    // restore start addr
    free(tp);
end_c:
    free(th_handles);
end_b:
    free(md.B);
end_a:
    free(md.A);
end_all:
    exit(ret); 
}