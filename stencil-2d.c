/**
 * @file stencil-2d.c
 * @author Leslie Horace
 * @brief Main program for performing and recording 9-pt serial stencil operations
 * @version 2.0
 * 
 */
#include "utilities.h"   

int stencilLoop(MatrixData md, FileData fd, StencilData * sd){
    size_t w_count = 0, m_count = MATRIX_COUNT(md.rows,md.cols);
    double start_compute=0.0, end_compute=0.0;
    FILE * fp;
    int ret = ERROR;

    if(fd.allfile != NULL){
         // check if file is open for writing
        if((fp = fopen(fd.allfile, "wb")) == NULL){ 
            printf("[stencil-2d:stencilLoop()]: cannot open/write '%s'\n", fd.allfile);
            goto stop_all;
        }
        // write initial matrix to stacked raw file and check for errors
        w_count = fwrite(md.A, DOUBLE_SIZE, m_count, fp);
        if(handleIOError(fp, w_count, m_count, "[stencil-2d:stencilLoop()]") == ERROR) goto stop_write;
    }
    // write the initial matrix to output file 
    for(int i=0; i < sd->iterations; i++){   
        GET_TIME(start_compute);      
        for(int i = 1; i < md.rows-1; i++){ 
            stencil2D(md.A, md.B, i, md.cols);        // perform stencil iterations 
        }
        GET_TIME(end_compute);
        sd->compute_time += (end_compute-start_compute);         // record/sum io time 
        // write current iteration to raw file, stop if error occurs
        if(fd.allfile != NULL){
            w_count = fwrite(md.A, DOUBLE_SIZE, m_count, fp);
            if(handleIOError(fp, w_count, m_count,"[stencil-2d:stencilLoop()]") == ERROR) goto stop_write;
        }
        // swap matrices for next iteration
        swap2D(&md.A, &md.B);  
    } 

    if(write2D(md.B, md.rows, md.cols, fd.finalfile)==ERROR) goto stop_write;

    ret=SUCCESS;
stop_write: 
    if(fd.allfile != NULL) free(fp);
stop_all:
    return ret;
}

int main(int argn, char **argv) {
    int ret = EXIT_FAILURE; 
    double start_time = 0.0, end_time = 0.0;
    GET_TIME(start_time); 

    if (argn < 4  || argn > 5){
        printf("Usage: %s <num_iterations> <infile> <outfile> <all_stacked_file(optional)>\n", argv[0]);
        goto end_all;
    }

    MatrixData md = {NULL, NULL, 0, 0};  
    FileData fd = {argv[2], argv[3], (argn > 4) ? argv[4] : NULL};
    StencilData sd = {0, 0, 0.0};

    // parse <num iterations> arg as base 10 int
    if((sd.iterations = parseInt(argv[1], 1, SKIP_ARG, "sd.iterations")) == ERROR) goto end_all;
    // check if reading file into md.A was successful
    if(read2D(&md.A, &md.rows, &md.cols, fd.initfile) == ERROR) goto end_a;
    // allocate space matrix md.B
    if(malloc1D((void*)&md.B, MATRIX_SIZE(md.rows, md.cols), "md.B") == ERROR) goto end_a;
    // initialize md.B as a duplicate of md.A
    init2D(md.B, md.rows, md.cols);    
    // perfrom stencil iterations
    printf("Running %d serial stencil iterations...\n", sd.iterations);
    if(stencilLoop(md, fd, &sd) == ERROR) goto end_b;
    // print file information
    printDataFileInfo(fd.finalfile, md.rows, md.cols, 0);
    if(fd.allfile != NULL) printStackedFileInfo(fd.allfile, md.rows, md.cols, sd.iterations);
    // calculate total time and cpu time, display total times for elapsed, compute, and io
    GET_TIME(end_time);

    printTimes((end_time-start_time), sd.compute_time);
    ret = EXIT_SUCCESS;

end_b:
    free(md.B);     
end_a:
    free(md.A);
end_all:
    exit(ret); 
}