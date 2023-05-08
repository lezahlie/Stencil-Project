/**
 * @file mpi_utils.h
 * @authors Leslie Horace, Jmaes Simmons
 * @brief Stores all helper and MPI functions used in mpi-floyds.c
 * @version 1.0 
 */

#include "mpi_utils.h"

void terminate(int code){
   MPI_Finalize();   // clean mpi env
   exit(code);   
}

void abortComm(int rank, char * err_msg, int err_code){
   if(err_msg != NULL) printf ("\nError: %s for process %d\n", err_msg, rank);
   FLUSH_OUTPUT
   MPI_Abort(MPI_COMM_WORLD, err_code);    // abort all processes in comm handle
}

int handleMpiError(int rank, int err_code, char * location){
   // if no error return
   if(err_code == MPI_SUCCESS)  return 0;
   // here if error returned from MPI call
   int class=0, size=0;
   char err_msg[MPI_MAX_ERROR_STRING]={0}; 
   MPI_Error_class(err_code, &class); 	// get error class 
   MPI_Error_string(err_code, err_msg, &size);	// get error class msg and display
   printf("Error [%s]: rank = %d, errcode = %d, %s\n", location, rank, err_code, err_msg);
   FLUSH_OUTPUT
   return err_code;
}

int setScatterData(ProcessData * pd, MatrixPointer * mp, StencilData * sd, ConditionBools cb){
   int share_data[4], ret = ERROR;
   if(cb.is_root){
      // set up offsets and displacesmments for scatter
      for(int i = 0; i < pd->num_p; i++){
         mp->sub_offset[i] = MATRIX_COUNT(BLOCK_LOW(i, pd->num_p, pd->rows-2), pd->cols);
         mp->sub_count[i] = MATRIX_COUNT(BLOCK_SIZE(i, pd->num_p, pd->rows-2)+2, pd->cols);
      }
      // data to share with other processes
      share_data[0] = pd->rows; share_data[1]= pd->cols; share_data[2] = sd->iterations, share_data[3] = sd->debug_level;
   }
   // send data to other processes 
   ret = MPI_Bcast(&share_data, 4, MPI_INT, pd->num_p-1, MPI_COMM_WORLD);

   if(handleMpiError(pd->rank, ret, "mpi-stencil-2d:main:MPI_Bcast(pd->rows)") != ERROR){
      // save data to lcoal struct and local block size
      pd->rows = share_data[0]; pd->cols = share_data[1]; sd->iterations = share_data[2]; sd->debug_level = share_data[3];
      pd->block_size = BLOCK_SIZE(pd->rank, pd->num_p, pd->rows-2)+2;
   }
   return ret;
}





