/**
 * @file print-2d.c
 * @author Leslie Horace
 * @brief Main program to read a matrix from and file print it's contents to the console
 * @version 2.0
 * 
 */
    
#include "utilities.h"

int main(int argn, char **argv) {
    int ret = EXIT_FAILURE;
    // check if all args were entered 
    if (argn != 2) {       
        printf("Usage: %s <input_data_file>\n", argv[0]);
        goto end;
    }
    int m = 0, n = 0; 
    char * infile = argv[1];
    double * A = NULL;

    // read in matrix from (.dat) file
    printf("Reading data from '%s'\n", infile);
    if(read2D(&A, &m, &n, infile) == ERROR) goto end;
    print2D(A, m, n);       // print the matrix
    free(A);

    ret = EXIT_SUCCESS;

end:
    exit(ret); 
}
