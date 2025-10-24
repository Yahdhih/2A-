#include <stdio.h>
#include <mpi.h>

int main(int argc, char **argv)
{
    int rk, jeton, tag1;
    MPI_Status sta;

    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rk);

    tag1 = 1000;
    if (rk == 0)
    {
        jeton = 10;
        MPI_Send(&jeton, 1, MPI_INT, 1, tag1, MPI_COMM_WORLD);
    }
    else if (rk == 1)
    {
        MPI_Recv(&jeton, 1, MPI_INT, 0, tag1, MPI_COMM_WORLD, &sta);

        printf("Je suis P%d et j'ai recu la valeur %d\n", rk, jeton);
    }

    MPI_Finalize();

    return 0;
}

