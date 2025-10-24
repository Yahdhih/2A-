#include <stdio.h>
#include <mpi.h>
#include <unistd.h>

void work(int iter)
{
    sleep(iter);
}

int main(int argc, char **argv)
{
    int rank, nsecs, flag, niter;
    MPI_Request req;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (rank == 0)
    {
        nsecs = 10;
        sleep(nsecs);

        MPI_Send(&nsecs, 1, MPI_INT, 1, 1000, MPI_COMM_WORLD);
    }
    else if (rank == 1)
    {
        MPI_Irecv(&nsecs, 1, MPI_INT, 0, 1000, MPI_COMM_WORLD, &req);

        niter = 0;
        do
        {
            printf("P1 : niter=%d\n", niter); fflush(stdout);
            niter++;
            work(niter);

            MPI_Test(&req, &flag, MPI_STATUS_IGNORE);
        }
        while(!flag);

        printf("Nsecs = %d s  / Nb iter = %d\n", nsecs, niter);
    }

    MPI_Finalize();
    return 0;
}

