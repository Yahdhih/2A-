#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>

#include "FreeImage.h"
#include "kernel.h"

int main (int argc, char **argv)
{
  MPI_Init(&argc, &argv);

  // TODO : récupérer le nombre de processus et le rang du processus
  int mpirank, mpisize;
  MPI_Comm_rank(MPI_COMM_WORLD, &mpirank);
  MPI_Comm_size(MPI_COMM_WORLD, &mpisize);

  // Check argument
  if (argc < 5)
  {
    fprintf(stderr, "Usage: %s [INPUT.jpg] [red] [green] [blue] \n", argv[0]);
    // Exit failure
    MPI_Abort(MPI_COMM_WORLD, 1);
  }


  // TODO : Couleur rgb à lire par le processus 0 et à diffuser
  int rgb[N_COMPONENT];

  if (mpirank == 0)
  {
    rgb[0] = atoi(argv[2]);
    rgb[1] = atoi(argv[3]);
    rgb[2] = atoi(argv[4]);
  }

  ??;
  // A partir d'ici, tous les processus ont les mêmes valeurs dans rgb
  printf("P%d, rgb=[%d %d %d]\n", mpirank, rgb[0], rgb[1], rgb[2]);


  // Cleanup !
  MPI_Finalize();

  return 0;
}

