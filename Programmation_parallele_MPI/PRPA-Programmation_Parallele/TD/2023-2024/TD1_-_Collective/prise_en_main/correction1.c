#include <stdio.h>
#include <mpi.h>
#include <unistd.h>

int main(int argc, char **argv) {

    int rang, P, len;
    pid_t pid;
    char str_host[MPI_MAX_PROCESSOR_NAME];

    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rang);
    MPI_Comm_size(MPI_COMM_WORLD, &P);

    MPI_Get_processor_name(str_host, &len);
    pid = getpid();
    printf("%d, %d, %s, pid=%d\n", rang, P, str_host, pid);

    MPI_Finalize();

    return 0;
}

