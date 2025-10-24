/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2004 by University of Chicago.
 *      See COPYRIGHT in top-level directory.
 */
#include <mpi.h>

#include "mlife.h"

// Communicator dedicated to exchanges
static MPI_Comm exch_comm = MPI_COMM_NULL;
static int exch_prev, exch_next;

/* MLIFE_exchange_init
 *
 * Parameters:
 * comm   - communicator describing group of processes
 * matrix - pointer to original matrix data
 * temp   - pointer to second region used during calculations
 * myrows - number of rows held locally
 * rows   - number of rows in global matrix
 * cols   - number of columns in global matrix
 * prev   - rank of processor "above" this one
 * next   - rank of processor "below" this one
 *
 * Note: Some of these parameters are not used by this exchange
 *       implementation.
 */
int MLIFE_exchange_init(MPI_Comm comm, void *matrix, void *temp,
                        int myrows, int rows, int cols,
                        int prev, int next)
{
// TODO 1 Duplicate the communicator comm into exch_comm, isolating 
// the following communications from possibly on-going communications

    exch_prev = prev;
    exch_next = next;

    return MPI_SUCCESS;
}

void MLIFE_exchange_finalize(void)
{
// Free the exchange specific communicator
    MPI_Comm_free(&exch_comm);
}

// ROW EXCHANGES
int MLIFE_exchange(int **matrix, int myrows, int cols)
{
    // TODO 1 Create a table of requests
    // ISend first domain specific row to previous process
    // IReceive upper ghost row from previous process
    // ISend last domain specific row to next process
    // IReceive lower ghost from previous process
    // Wait for the all 4 requests

    return MPI_SUCCESS;
}

/* END OF EXAMPLE */
