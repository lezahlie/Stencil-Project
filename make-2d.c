/**
 *  @file make-2d.c
 *  @author Leslie Horace
 *  @brief Main program to create a matrix and write it to a file
 *  @version 1.0
 * 
 */
#include "utilities.h"

int main(int argn, char **argv) {
    int ret = EXIT_FAILURE;
    // check if all args were entered 
    if (argn != 4) { 
        printf("Usage: %s <num_rows> <num_cols> <output data file>\n", argv[0]);
        goto end;
    }
    int m = 0, n = 0;
    // check if <num_rows> <num_cols> are valid
    if((m = parseInt(argv[1], 3, SKIP_ARG, "num_rows")) == ERROR) goto end;
    if((n = parseInt(argv[2], 3, SKIP_ARG, "num_cols")) == ERROR) goto end;

    double *A;
    char * initfile = argv[3];
    
    // allocate space for A
    if(malloc1D((void*)&A, MATRIX_SIZE(m,n), "A") == ERROR) goto end;
    init2D(A, m, n);    // initialize A 
    if(write2D(A, m, n, initfile) == ERROR) goto end;    // write A to output file   
    printDataFileInfo(initfile, m, n, 0);     // display info and deallocate mem
    ret = EXIT_SUCCESS;
    free(A);
end:  
    exit(ret);
}



