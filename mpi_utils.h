/**
 * @file mpi_utils.h
 * @authors Leslie Horace, James Simmons
 * @brief Header file for macros and functions in mpi_utils.c
 * @version 1.0 
 */

#include <mpi.h>
#include "utilities.h"

#ifndef MPI_UTILS_
#define MPI_UTILS_

#define FLUSH_OUTPUT fflush(stdout);    // flushs stdout buffer stream
#define EQUAL(a,b) ((a)==(b) ? 1 : 0)
#define NOT_EQUAL(a,b) ((a)!=(b) ? 1 : 0)
#define IS_EVEN(n) ((n)%2==0 ? 1 : 0)
#define LEFT(rank) ((EQUAL(rank,0)) ? MPI_PROC_NULL : rank-1)    // left neighbor 
#define RIGHT(rank, first) ((first) ? MPI_PROC_NULL : rank+1)     // right neighbor

#define TOP_SOURCE(n) n                                 // rank send top row
#define BOT_SOURCE(m,n) MATRIX_COUNT((m-2),(n))         // rank send bottom row
#define TOP_TARGET 0                                    // rank recv top row (ghost)
#define BOT_TARGET(m,n) MATRIX_COUNT((m-1),(n))         // rank recv bottom row (ghost)

/** 
 *  @struct _processData
 *  @typedef ProcessData (local)
 *  @brief struct for all local data needed by each process 
 */
typedef struct _processData{
    int rank;
    int num_p;
    int block_size;
    int rows;
    int cols;
}ProcessData;

typedef struct _conditionBools{       
    int is_root;
    int is_parallel;
    int write_state;
    int print_state;
    int debug_on;
}ConditionBools;

typedef struct _matrixPointer{       
    double * A;
    double * B;
    double * C;
    int * sub_offset;
    int * sub_count;
}MatrixPointer;


/** 
 *  @brief terminates mpi execution, cleans up mpi env, and exits
 *  @param err_code (int) error code to exit with
 */
void terminate(int err_code);

/** 
 *  @brief terminates all communicating processes
 *  @param id (int) process id
 *  @param err_message (char *) custom error message
 *  @param err_code (int) error code to abort with
 */
void abortComm(int id, char * err_msg, int err_code);

/** 
 *  @brief  allows handling mpi errors after getting useful error info
 *  @param id (int) process rank
 *  @param err_code (int) err_code to find MPI error class info
 *  @return [value]: 0 = MPI_SUCCESS | MPI error code
 */
int handleMpiError(int rank, int err_code, char * location);

/** 
 *  @brief computes offsets and displacements for MPI_Scatterv
 *  @param sub_offset (int[]) vla for sub offset MPI_Scatterv
 *  @param sub_offset (int[]) vla for sub counts in MPI_Scatterv
 *  @param pd (ProcessData *) local struct for process data
 *  @param sd (StencilData *) local struct for stencil data
 *  @return [value]: -1 = ERROR | 0 = SUCCESS; [args]: sub_offset, sub_count, pd, sd;
 */
int setScatterData(ProcessData * pd, MatrixPointer * mp, StencilData * sd, ConditionBools cb);

#endif /* MPI_UTILS_ */