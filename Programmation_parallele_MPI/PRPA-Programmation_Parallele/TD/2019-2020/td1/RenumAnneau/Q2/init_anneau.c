#include <stdio.h>
#include <mpi.h>

int main(int argc, char **argv)
{
    int rang, nproc, est, ouest, me;

    MPI_Init(&argc, &argv);

    /* Initialisation des données du problème
       */
    MPI_Comm_rank(MPI_COMM_WORLD, &rang);
    MPI_Comm_size(MPI_COMM_WORLD, &nproc);
    est   = (rang+1) % nproc;
    ouest = (rang+nproc-1) % nproc;
    me = (rang == 0 ? 0 : -1);

    printf("P%d : me = %d, ouest = %d, est = %d\n", rang, me, ouest, est);

    MPI_Finalize();

    return 0;
}

