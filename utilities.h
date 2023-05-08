/**
 *  @file utilities.h
 *  @author Leslie Horace
 *  @brief Header file for structs and functions in functions.n
 *  @version 2.0 
 * 
 */
#define _GNU_SOURCE
#include <stdlib.h>     
#include <stdio.h>  
#include <pthread.h>
#include <errno.h>
#include "timer.h"
#ifndef UTILITIES_
#define UTILITIES_

#define DOUBLE_SIZE sizeof(double)
#define INT_SIZE sizeof(int)
#define PTR_SIZE sizeof(void*)

#define MAX(a,b)  ((a)>(b)?(a):(b))                                     // max value between a and b
#define BLOCK_LOW(id,p,m)  ((id)*(m))/(p)                               //  block starting index 
#define BLOCK_HIGH(id,p,m) (BLOCK_LOW((id)+1,p,m)-1)                    //  block ending index
#define BLOCK_SIZE(id,p,m) (BLOCK_HIGH(id,p,m)-BLOCK_LOW(id,p,m)+1)     //  block size 
#define IDX(i,j,n) (i)*(n)+(j)

#define MATRIX_COUNT(m,n) ((long)(m)*(long)(n))                 // matrix element count
#define MATRIX_SIZE(m,n) (MATRIX_COUNT(m,n)*DOUBLE_SIZE)        // matrix size in bytes
#define DATAFILE_SIZE(m,n) (MATRIX_SIZE(m,n)+(2*INT_SIZE))      // calculates data file size
#define RAWFILE_SIZE(m,n,i) (MATRIX_SIZE(m,n)*(i+1))            // calculates raw file size



#define BtoGB(bytes) (long double)(bytes*(0.1*10e-9))   // converts bytes to GB for debugging

#define SUCCESS 0       // universal success code for child functions
#define ERROR -1        // universal error code for child functions
#define SKIP_ARG -2     // const to skip args in error handling func

/** 
 *  @struct _matrixData
 *  @typedef MatrixData (shared)
 *  @brief  struct for all shared variables for matrices
 */
typedef struct _matrixData{
    double *A;
    double *B;
    int rows;
    int cols;
}MatrixData;

/** 
 *  @struct _fileData
 *  @typedef FileData (shared)
 *  @brief  struct for all shared file name handlers
 */
typedef struct _fileData{
    char * initfile;
    char * finalfile;
    char * allfile;
}FileData;

/** 
 *  @struct _stencilData
 *  @typedef StencilData (shared)
 *  @brief  struct for all shared variables needed in stencil interation loop
 */
typedef struct _stencilData{
    int iterations;
    int debug_level;
    double compute_time;
}StencilData;

/** 
 *  @struct _threadShared
 *  @typedef ThreadShared (shared)
 *  @brief  struct for all shared variables needed by each thread
 */
typedef struct _threadShared{
    int num_threads;
    pthread_barrier_t barrier;
}ThreadShared;

/** 
 *  @struct _ThreadPrivate
 *  @typedef ThreadPrivate (private)
 *  @brief thread struct holding shared struct pointers and private variables
 */
typedef struct _threadPrivate{
    MatrixData *m_data;
    FileData *f_data;
    StencilData *s_data;              
    ThreadShared *t_shared;
    int rank;             
    int block_start;
    int block_size;
    double thread_compute; 
}ThreadPrivate;


/** 
 *  @brief Allocates space for any type 1D array 
 *  @param P (void**) Matrix to initalize
 *  @param size (long) size to malloc [n*sizeof(type)]
 *  @param p_name (char*) pointer name
 *  @return [arg] = P (addr); [val]: ERROR (-1) | SUCCESS (0)
 */
int malloc1D(void**P, long size, char * p_name);


/**
 *  @brief Initilaizes a matrix in memory for 9-pt stencil operations
 *  @param X (double**) Matrix to initalize
 *  @param m (int) # rows
 *  @param n (int) # columns
*/
void init2D(double *X, int r, int n);

/**
 *  @brief Prints a matrix to the console
 *  @param X (double*) Matrix to be printed 
 *  @param m (int) # rows
 *  @param n (int) # columns
*/
void print2D(double *X, int r, int n);

/**
 *  @brief Swaps addresses of two matrix pointers
 *  @param X (double**) Source matrix 
 *  @param Y (double**) Target matrix 
 *  @return [arg] X (addr), Y (addr)
*/
void swap2D(double **X, double ** Y);


/**
 *  @brief Reads matrix from a data file as binary into memory
 *  @param X (double**) Matrix for reading
 *  @param m (int*) # rows
 *  @param n (int*) # columns
 *  @param infile (char*) Input filename (.dat)
 *  @return [arg] X (addr), n (addr), m (addr); [val]: ERROR (-1) | SUCCESS (0) 
 */
int read2D(double **X, int *m, int *n, char * infile);

/**
 *  @brief Writes matrix from memory as binary data into a (.dat) file
 *  @param X (double*) Matrix for writing
 *  @param m (int) # rows
 *  @param n (int) # columns
 *  @param outfile Input filename (.dat)
 *  @return [val]: ERROR (-1) | SUCCESS (0)
 */
int write2D(double *A, int m, int n, char * outfile);

/**
 *  @brief Algorithm to perform a 9-pt stencil operation
 *  @param X (double*) Matrix being modified
 *  @param Y (double*) Matrix for computations
 *  @param m (int) # rows
 *  @param n (int) # columns
 */
void stencil2D(double *X, double *Y, int ri, int n);

/**
 *  @brief Validates integer arguments from cmdline
 *  @param arg_ptr (char*) integer argument to parse
 *  @param arg_min (int) min valid integer (SKIP_ARG = NULL)
 *  @param arg_max (int) max valid integer (SKIP_ARG = NULL)
 *  @param arg_name (char*) argument usage name
 *  @return [val]:  parsed result (int) | ERROR (-1)
 */
int parseInt(char *arg_ptr, int arg_min, int arg_max, char *arg_name);

/**
 *  @brief Check if valid thread reached the call to pth_barrier func()
 *  @param retval (int) return value from pth_barrier func()
 *  @param location (char *) return value from pth_barrier func()
 *  @return [val]: ERROR (-1) | SUCCESS (0)
*/
int handleBarrier(int retval, char * location);

/**
 *  @brief Check for specific file errors for fwrite and fread
 *  @param fptr (FILE*) file stream pointer to check
 *  @param count (size_t) count returned from fread() or fwrite()
 *  @param expected (size_t) count of elements read orwritten
 *  @param location (char*) calling file and function of the error
 *  @return [val]: ERROR (-1) | SUCCESS (0)
*/
int handleIOError(FILE * fptr, size_t count, size_t expected, char * location);

/**
 *  @brief prints file information for inital and final matrix
 *  @param file_name (char*) output data filename
 *  @param m (int) # rows
 *  @param n (int) # cols
 *  @param state (int) matrix state: inital = 0, final = 1
*/
void printDataFileInfo(char * file_name, int m, int n, int state);

/**
 *  @brief prints file information for raw file with all iterations
 *  @param file_name (char*) output stacked filename
 *  @param m (int) # rows
 *  @param n (int) # cols
 *  @param iter (int) # interations
*/
void printStackedFileInfo(char * file_name, int m, int n, int iter);

/**
 *  @brief prints timing info ffor Overall, I/O, and Compute
 *  @param overall_time (double) overall time
 *  @param compute_time (double) computation time
*/
void printTimes(double overall_time, double compute_time);


#endif /* UTILITIES_ */