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
  if (argc < 3)
  {
    fprintf(stderr, "Usage: %s [INPUT.jpg] [OUTPUT.jpg] \n", argv[0]);
    // Exit failure
    exit(1);
  }

  FreeImage_Initialise(0); // A NE PAS RETIRER

  char PathName[256] = "img.jpg";
  char PathDest[256] = "new_img.png";

  strcpy(PathName, argv[1]);
  strcpy(PathDest, argv[2]);


  image_t src, dst;

  // TODO : seul le processus 0 lit le fichier source d'image
  if (mpirank == 0)
  {
    // Après cet appel, src est rempli
    read_image_from_file(PathName, &src);

    // TODO : vérification que src.height est un multiple de mpisize
    if (src.height%mpisize != 0)
    {
      fprintf(stderr, "Le nb de lignes de l'image %s (=%d) n'est pas un multiple du nb de processus MPI (=%d) !\n",
	  PathName, src.height, mpisize);

      MPI_Abort(MPI_COMM_WORLD, 1); 
    }
  }
  else
  {
    // Seul le processus 0 a un buffer alloué et rempli dans src.buf 
    // (et la fonction MPI_Scatter ne lira ce pointeur que sur le processus root 0)
    src.buf = NULL;
  }

  // Diffusion des paramètres de l'image de 0 vers tous
  MPI_Bcast(&src.width , 1, MPI_INT, 0, MPI_COMM_WORLD);
  MPI_Bcast(&src.height, 1, MPI_INT, 0, MPI_COMM_WORLD);
  MPI_Bcast(&src.pitch , 1, MPI_INT, 0, MPI_COMM_WORLD);
  MPI_Bcast(&src.bpp   , 1, MPI_INT, 0, MPI_COMM_WORLD);
  // A partir d'ici, tous les processus ont les mêmes valeurs de src.width, src.height


  // Recopie des paramètres de src dans dst
  dst.width  = src.width ;
  dst.height = src.height/mpisize; // attention, l'image dst est plus petite que l'image src
  dst.pitch  = src.pitch ;
  dst.bpp    = src.bpp   ;

  // Calcul de dst.sz_in_bytes et allocation de dst.buf à partir des paramètres dst.width, dst.height, ...
  allocate_image_from_parameters(&dst);

  /**************************/
  /* TODO : Distribution des sous-parties de l'image src à dst */
  /**************************/
  MPI_Scatter(
      src.buf, dst.sz_in_bytes, MPI_BYTE, // ces arguments sont ignorés pour tout processus !=0 (ie !=root)
      dst.buf, dst.sz_in_bytes, MPI_BYTE,
      /*root=*/0, MPI_COMM_WORLD
      );


  /*******/
  /* End */
  /*******/
  // TODO : Sauvegarde de l'image dans un fichier avec un suffixe avec le rang MPI
  char suffix[32];
  sprintf(suffix, "%d", mpirank);
  write_image_into_file(&dst, PathDest, suffix);

  free_image(&src);
  free_image(&dst);

  // Cleanup !
  MPI_Finalize();
  FreeImage_DeInitialise(); // A NE PAS RETIRER

  return 0;
}

