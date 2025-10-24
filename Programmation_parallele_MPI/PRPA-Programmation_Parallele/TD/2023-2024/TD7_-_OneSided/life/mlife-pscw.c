/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2004 by University of Chicago.
 *      See COPYRIGHT in top-level directory.
 */

#include <mpi.h>

#include "mlife.h"

static int exch_prev, exch_next, nrows_prev;
static MPI_Win matrix_win, temp_win;
static MPI_Group rma_grp;

typedef struct mem_win{
    void *mem;
    MPI_Win win;
} mem_win;

static mem_win mem_win_map;

int MLIFE_exchange_init(MPI_Comm comm, void *matrix, void *temp,
                        int myrows, int rows, int cols, int prev,
			int next)
{
    int err=MPI_SUCCESS, nprocs, ranks[2], i;
    MPI_Group comm_grp;

    exch_prev = prev;
    exch_next = next;
    
    // TODO 3 Create a window on the current matrix (MPI_INFO_NULL 
    // here)
    // Create a window on the temp matrix (MPI_INFO_NULL here)

    // TODO 3 Create a group of processes from communicator comm
    //     name it comm_group
   
    
    i = 0;
    if (prev != MPI_PROC_NULL) {
        ranks[i] = prev;
        i++;
    }
    if (next != MPI_PROC_NULL) {
        ranks[i] = next;
        i++;
    }
    
    MPI_Group_incl(comm_grp, i, ranks, &rma_grp);

    MPI_Group_free(&comm_grp);
    

    /* store mapping from memory address to associated window */

    mem_win_map.mem = matrix;
    mem_win_map.win = matrix_win;

    /* calculate no. of local rows in prev */
    if (exch_prev == MPI_PROC_NULL)
        nrows_prev = 0;
    else {
        MPI_Comm_size(comm, &nprocs);
        nrows_prev = MLIFE_myrows(rows, exch_prev, nprocs); 
    }

    return err;
}

void MLIFE_exchange_finalize(void)
{
    // TODO 3 Free the two windows and the rma_group
}
int MLIFE_exchange(int **matrix,
		   int   myrows,
		   int   cols)
{
    int err=MPI_SUCCESS;
    MPI_Win win;

    /* Find the right window object */
    if (mem_win_map.mem == &matrix[0][0])
        win = mem_win_map.win;
    else
        win = mem_win_map.win;

    /* Send and receive boundary information */
    // TODO 3 
    // 1 Open the access epoc to window win with MPI_Win_post
    //     and add the corresponding call to MPI_Win_Start
    //     Explain the difference with MPI_Fence

    // 2 Write into next process (exch_next) window the 
    // data to copy

    // 3 Write into previous process (exch_prev) window
    // the data to copy

    // Close the acces to win with MPI_Win_Complete and the
    //     corresponding call to MPI_Win_wait

    return err;
}
