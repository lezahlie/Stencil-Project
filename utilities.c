/**
 * @file utilities.c
 * @author Leslie Horace
 * @brief File storing all user functions used in the project
 * @version 2.0
 * 
 */
#include "utilities.h"


int malloc1D(void**P, long size, char * p_name){
    void *Q = (void*)malloc(size);   // malloc an array of pointers
    if (Q == NULL){ // check if null and print error
        printf("Error [utilities:malloc1D:malloc()]: cannot allocate space for %s[%ld]\n", p_name, size);
        return ERROR;
    }*P=Q;  // here if no error, set start addr 
    return SUCCESS;
}

void init2D(double *X, int m, int n){
    long r = (long)m, c = (long)n; 
    for(long i = 0; i < r; i++){
        for(long j = 1; j < c-1; j++){
            X[IDX(i,j,c)] = 0;        // set each middle col to 0
        }
        X[IDX(i,0,c)] = X[IDX(i,c-1,c)] = 1;        // set first and last col to 1
    }
}

void print2D(double *X, int m, int n){
    long r = (long)m, c = (long)n; 
    printf("\n");
    for(long i = 0; i < r; i ++){
        for(long j = 0; j < c; j ++){
            printf("%.2f\t", X[IDX(i,j,c)]);     // prints each element
        }printf("\n");
    }
}

void swap2D(double **X, double **Y){
    double *Z = *Y;        // temp pointer = target matrix
    *Y=*X;      // target = source
    *X=Z;       // source = target
}

int read2D(double **X, int *m, int *n, char * infile){
    FILE * fp = NULL; 
    size_t read_count = 0;
    int ret = ERROR;

    // check if file is open for reading
    if ((fp = fopen(infile, "rb")) == NULL){
        printf("Error [utilities:read2D:fopen()]: cannot open/read '%s'\n", infile);
        goto end_all;
    }
    // attempt to read in matrix order metadata
    read_count = fread(&(*m), INT_SIZE, 1, fp);
    if(handleIOError(fp, read_count, (size_t)1, "Error [utilities:read2D:fread()]") == ERROR) goto end_read;
    read_count = fread(&(*n), INT_SIZE, 1, fp);
    if(handleIOError(fp, read_count, (size_t)1, "Error [utilities:read2D:fread()]") == ERROR) goto end_read;

    // attempt to allocate space for n*n natrix
    if(malloc1D((void*)X, MATRIX_SIZE(*m,*n), "Y") == ERROR) goto end_read;

    // attempt to read infile contents into matrix 
    read_count = fread(X[0], DOUBLE_SIZE, MATRIX_COUNT(*m,*n), fp); 
    if(handleIOError(fp, read_count, (size_t)MATRIX_COUNT(*m,*n), "Error [utilities:read2D:fread()]") == ERROR){
        goto end_read; // deallocate and exit
    }

    ret = SUCCESS;

end_read:
    fclose(fp); 
end_all:
    return ret;
}

int write2D(double *X, int m, int n, char * outfile){
    FILE * fp = NULL; 
    int ret = ERROR;
    size_t write_count = 0;

    // check if file is open for writing
    if ((fp = fopen(outfile, "wb")) == NULL){
        printf("Error [utilities:write2D:fopen()]: cannot open/write '%s'\n", outfile);
        goto end_all;
    }
    // write matrix order metadata
    write_count = fwrite(&m, INT_SIZE, 1, fp);
    if(handleIOError(fp, write_count, (size_t)1, "Error [utilities:write2D:fwrite()]") == ERROR) goto end_write;
    write_count = fwrite(&n, INT_SIZE, 1, fp);
    if(handleIOError(fp, write_count, (size_t)1, "Error [utilities:write2D:fwrite()]") == ERROR) goto end_write;

    // write matrix into outfile
    write_count = fwrite(X, DOUBLE_SIZE, MATRIX_COUNT(m,n), fp);
    if(handleIOError(fp, write_count, (size_t)MATRIX_COUNT(m,n), "Error [utilities:write2d:fwrite()] ") == ERROR) goto end_write;

    ret = SUCCESS;   // here if no error

end_write:   
    fclose(fp);
end_all:
    return ret;
}

void stencil2D(double *X, double *Y, int ri, int n){
    long i = (long)ri, c = (long)n;
    for(long j = 1; j < c-1; j++){
        X[IDX(i,j,c)] = (Y[IDX(i-1,j-1,c)] + Y[IDX(i-1,j,c)] + Y[IDX(i-1,j+1,c)] 
            + Y[IDX(i,j+1,c)] + Y[IDX(i+1,j+1,c)] + Y[IDX(i+1,j,c)] 
            + Y[IDX(i+1,j-1,c)] + Y[IDX(i,j-1,c)] + Y[IDX(i,j,c)])/9.0;
        
    }
    
}

int parseInt(char *arg_ptr, int arg_min, int arg_max, char *arg_name){
    char * tmp_ptr = NULL;  
    int tmp_num = 0, ret = ERROR;

    // parse the arg as base 10 int
    tmp_num = strtol((char *)arg_ptr, &tmp_ptr, 10);  
    // check if arg is an integer and at least the specified min 
    if (*tmp_ptr != '\0'){
        printf("Error [utilities:parse_int()]: <%s> %s is not an integer\n", arg_name, arg_ptr);
    }else if(arg_min != SKIP_ARG && tmp_num < arg_min){
        printf("Error [utilities:parse_int()]: <%s> %s must at least %d\n", arg_name, arg_ptr, arg_min);
    }else if(arg_max != SKIP_ARG && tmp_num > arg_max){
        printf("Error [utilities:parse_int()]: <%s> %s must at most %d\n", arg_name, arg_ptr, arg_max);
    }else ret = tmp_num;

    return ret;
}

int handleBarrier(int retval, char * location){
    // on success 1 thread returns the below macro and the rest return 0
    if(retval != PTHREAD_BARRIER_SERIAL_THREAD && retval != SUCCESS){
        perror(location);
        return ERROR;
    }
    return SUCCESS;
}

int handleIOError(FILE * fptr, size_t count, size_t expected, char * location){
    // no error reading/writing
    if(expected != count) printf("Error %s: count returned [%ld] != expected [%ld]\n", location, count, expected);
    else if(ferror(fptr))  perror(location);
    else if(feof(fptr))  perror(location);
    else return SUCCESS;
    return ERROR;   
}

void printDataFileInfo(char * file_name, int m, int n, int state){
    printf("------------------------------------------------------\n");
    printf("Wrote %s matrix[%dx%d] state to '%s'\n[%s] size = %ld(B) = %.6Lg(GB)\n", 
        (state) ? "final" : "initial", m, n, file_name, file_name, DATAFILE_SIZE(m,n), BtoGB(DATAFILE_SIZE(m,n)));
}

void printStackedFileInfo(char * file_name, int m, int n, int iter){
    printf("------------------------------------------------------\n");
    printf("Wrote inital matrix state + %d iterations to '%s'\n[%s] size = %ld(B) = %.6Lg(GB)\n", 
        iter, file_name, file_name, RAWFILE_SIZE(m, n, iter), BtoGB(RAWFILE_SIZE(m,n,iter)));
}

void printTimes(double overall_time, double compute_time){
    printf("\n[Overall Time] = %g sec\n[I/O Time] = %g sec\n[Compute Time] = %g sec", 
        overall_time, overall_time-compute_time, compute_time);
    printf("\n------------------------------------------------------------\n");
}