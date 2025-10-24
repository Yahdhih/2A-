/* SLIDE: MPI-IO Life Checkpoint Code Walkthrough */
/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2004 by University of Chicago.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>

#include <mpi.h>

#include "mlife-io.h"

/* MPI-IO implementation of checkpoint and restart for MPI Life
 *
 * Data stored in matrix order, with a header consisting of three
 * integers: matrix size in rows and columns, and iteration no.
 *
 * Each checkpoint is stored in its own file.
 */

static int MLIFEIO_Type_create_rowblk(int **matrix, int myrows,
                                      int cols,
                                      MPI_Datatype *newtype);
static int MLIFEIO_Type_create_hdr_rowblk(int **matrix,
                                          int myrows,
                                          int *rows_p,
                                          int *cols_p,
                                          int *iter_p,
                                          MPI_Datatype *newtype);
/* SLIDE: MPI-IO Life Checkpoint Code Walkthrough */
static MPI_Comm mlifeio_comm = MPI_COMM_NULL;

int MLIFEIO_Init(MPI_Comm comm)
{
    int err;
    
    /* Zone-1-A communicator duplication : comm into mlifeio_comm (MPI_Comm_dup)
     */ 

    return err;
}

int MLIFEIO_Finalize(void)
{
    int err;

    /* Zone-1-A-bis communicator duplication : free mlifeio_comm communicator (MPI_Comm_free)
     */ 

    return err;
}

int MLIFEIO_Can_restart(void)
{
    return 1;
}

// Building the protection (save neccessary data in parallel)
       /* SLIDE: Life MPI-IO Checkpoint/Restart */
int MLIFEIO_Checkpoint(char *prefix, int **matrix, int rows,
                       int cols, int iter, MPI_Info info)
{
    int err;
    /* Zone-1-B define the correct opening mode for writing the protection file
     */ 
    int rank, nprocs;
    int myrows, myoffset;

    // MPI file object : in which protection will be written
    MPI_File fh;
    // MPI type object : defining the format of data to write into MPI_file
    MPI_Datatype type;
    // MPI offset object : defining the offset in file for each processe
    MPI_Offset myfileoffset;

    char filename[256];

    MPI_Comm_size(mlifeio_comm, &nprocs);
    MPI_Comm_rank(mlifeio_comm, &rank);

    myrows   = MLIFE_myrows(rows, rank, nprocs);
    myoffset = MLIFE_myrowoffset(rows, rank, nprocs);

    snprintf(filename, 255, "%s-%d.chkpt", prefix, iter);

    /* Zone 1-B-bis open a MPI_file fh named "filename", with amode (MPI_File_Open)
     */
    if (err != MPI_SUCCESS) {
        fprintf(stderr, "Error opening %s.\n", filename);
        return err;
    }

    // The process of rank 0 creates its own MPI_Type because he
    // needs to write a header containing additionnal information
    // (total number of rows, columns dans )
    if (rank == 0) {
/* SLIDE: Life MPI-IO Checkpoint/Restart */
        MLIFEIO_Type_create_hdr_rowblk(matrix, myrows, &rows, &cols, &iter, &type);
        /* Zone-1-E compute the file offset for process 0
         */
    }
    else {
        MLIFEIO_Type_create_rowblk(matrix, myrows, cols, &type);
        /* Zone-1-E-bis compute the file offset for non-0 processes
         */
    }

    /* Zone-1-E-ter commit the new type so that it can be used (MPI_Type_commit)
     */

    /* Zone-1-E-quater Explicitly write the data at the computed offset in file,
     * use MPI_File_write_at_all
     */

    /* Zone-1-E-quinquies free type (MPI_Type_free)
     */

    /* Zone 1-B-ter close a MPI_file fh (MPI_File_close)
     */
    return err;
}

       /* SLIDE: Life MPI-IO Checkpoint/Restart */
int MLIFEIO_Restart(char *prefix, int **matrix, int rows,
                    int cols, int iter, MPI_Info info)
{
    int err, gErr;
    /* Zone-2-A define the correct opening mode for reading the protection file
     */
    int rank, nprocs;
    int myrows, myoffset;
    int buf[3]; /* rows, cols, iteration */

    // MPI file object : in which protection will be written
    MPI_File fh;
    // MPI type object : defining the format of data to write into MPI_file
    MPI_Datatype type;
    // MPI offset object : defining the offset in file for each processe
    MPI_Offset myfileoffset;

    char filename[256];

    MPI_Comm_size(mlifeio_comm, &nprocs);
    MPI_Comm_rank(mlifeio_comm, &rank);

    myrows   = MLIFE_myrows(rows, rank, nprocs);
    myoffset = MLIFE_myrowoffset(rows, rank, nprocs);

    snprintf(filename, 255, "%s-%d.chkpt", prefix, iter);

    /* Zone 2-A-bis open a MPI_file fh named "filename", with amode (MPI_File_open)
     */
    if (err != MPI_SUCCESS) 
    {
        fprintf(stdout, " File opening failure\n");
        return err;
    }

    /* check that rows and cols match */
    /* Zone 2-B make all processes read the header of the file : it contains
     * 3 integers (rows, columns, iterations), use MPI_File_read_at_all
     */

    /* SLIDE: Life MPI-IO Checkpoint/Restart */
    /* Have all processes checking that nothing went wrong */
    /* Zone 2-B-bis use an allreduce max on err to check if there was an error
     * in the read.
     */

    if (gErr || buf[0] != rows || buf[1] != cols) 
    {
        if (rank == 0) 
        {
            fprintf(stderr, "restart failed.\n");
        }
        return MPI_ERR_OTHER;
    }

    // Now we build a type in order to read the matrix block associated to 
    // the process
    MLIFEIO_Type_create_rowblk(matrix, myrows, cols, &type);
    /* Zone 2-C compute the offset in the file (myfileoffset) to access the 
     * desired data (use myoffset)
     */

    /* Zone 2-C-bis commit the type and read the selected data (type and 
     * offset), use MPI_Type_commit and MPI_File_read_at_all
     */

    /* Zone 2-C-ter free type (MPI_Type_free)
     */

    /* Zone 2-A-ter close a MPI_file fh (MPI_File_close)
     */
    return err;
}

       /* SLIDE: Describing Header and Data */
/* MLIFEIO_Type_create_hdr_rowblk
 *
 * Used by process zero to create a type that describes both
 * the header data for a checkpoint and its contribution to
 * the stored matrix.
 *
 * Parameters:
 * matrix  - pointer to the matrix, including boundaries
 * myrows  - number of rows held locally
 * rows_p  - pointer to # of rows in matrix (so we can get its
 *           address for use in the type description)
 * cols_p  - pointer to # of cols in matrix
 * iter_p  - pointer to iteration #
 * newtype - pointer to location to store new type ref.
 */
static int MLIFEIO_Type_create_hdr_rowblk(int **matrix,
                                          int myrows,
                                          int *rows_p,
                                          int *cols_p,
                                          int *iter_p,
                                          MPI_Datatype *newtype)
{
    int err;
    int lens[4] = { 1, 1, 1, 1 };
    MPI_Aint disps[4];
    MPI_Datatype types[4];
    MPI_Datatype rowblk;

    MLIFEIO_Type_create_rowblk(matrix, myrows, *cols_p, &rowblk);

    /* Zone-1-D Create in newtype a MPI structure (MPI_Type_create_struct) 
     * It should contain three integers and a rowblk (MPI_Get_address, MPI_BOTTOM).
     */

    /* Zone-1-D-bis free rowblk (MPI_Type_free)
     */

    return err;
}

       /* SLIDE: Placing Data in Checkpoint */
/* MLIFEIO_Type_create_rowblk
 *
 * See stdio version for details (this is a copy).
 */
static int MLIFEIO_Type_create_rowblk(int **matrix, int myrows,
                                      int cols,
                                      MPI_Datatype *newtype)
{
    int err, len;

    MPI_Datatype vectype;
    MPI_Aint disp;

    /* since our data is in one block, access is very regular */
    /* Zone-1-C Create in vectype a MPI_Type_vector for the desired portion of the matrix
     */
    if (err != MPI_SUCCESS) return err;

    /* wrap the vector in a type starting at the right offset */
    len = 1;
    /* Zone-1-C-bis Position vectype so that it covers the desired data in the matrix (MPI_Get_address, MPI_Type_create_hindexed)
     */

    /* Zone-1-C-ter Free vectype (MPI_Type_free)
     */

    return err;
}
