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
  if (argc < 6)
  {
    fprintf(stderr, "Usage: %s [INPUT.jpg] [red] [green] [blue] [OUTPUT.jpg] \n", argv[0]);
    // Exit failure
    MPI_Abort(MPI_COMM_WORLD, 1);
  }

  FreeImage_Initialise(0); // A NE PAS RETIRER

  // TODO : Couleur rgb à lire par le processus 0 et à diffuser
  int rgb[N_COMPONENT];

  if (mpirank == 0)
  {
    rgb[0] = atoi(argv[2]);
    rgb[1] = atoi(argv[3]);
    rgb[2] = atoi(argv[4]);
  }

  MPI_Bcast(rgb, N_COMPONENT, MPI_INT, 0, MPI_COMM_WORLD);
  // A partir d'ici, tous les processus ont les mêmes valeurs dans rgb
  //printf("P%d, rgb=[%d %d %d]\n", mpirank, rgb[0], rgb[1], rgb[2]);

  char PathName[256] = "img.jpg";
  char PathDest[256] = "new_img.png";

  strcpy(PathName, argv[1]);
  strcpy(PathDest, argv[5]);


  image_t src, dst;

  // TODO : seul le processus 0 lit le fichier source d'image
  if (mpirank == 0)
  {
    // Après cet appel, src est rempli
    read_image_from_file(PathName, &src);
  }
  else
  {
    // Seul le processus 0 a un buffer alloué et rempli dans src.buf 
    // (et la fonction MPI_Scatterv ne lira ce pointeur que sur le processus root 0)
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
  dst.pitch  = src.pitch ;
  dst.bpp    = src.bpp   ;

  // TODO : La valeur de dst.height va dépendre du rang
  // Chaque processus va travailler sur les [glob_yfirst, glob_ylast[ de l'image d'origine
  int glob_yfirst = (mpirank*src.height)/mpisize;
  int glob_ylast = ((mpirank+1)*src.height)/mpisize;
  dst.height = glob_ylast - glob_yfirst;

  // Calcul de dst.sz_in_bytes et allocation de dst.buf à partir des paramètres dst.width, dst.height, ...
  allocate_image_from_parameters(&dst);

  /**************************/
  /* TODO : Distribution des sous-parties de l'image src à dst */
  /**************************/
  // Le processus root 0 doit déterminer les tailles en octets à envoyer à chaque processus ainsi
  // que les positions d'origine
  int *sndcounts, *displs;

  if (mpirank == /*root*/0)
  {
    sndcounts = (int*)malloc(mpisize*sizeof(int));
    displs    = (int*)malloc(mpisize*sizeof(int));

    // pour convertir en octets, taille en octets d'une ligne
    int in_bytes = sizeof(unsigned char) * N_COMPONENT * src.width; 
    for(int r=0 ; r<mpisize ; r++)
    {
      int gyfirst_bytes = (( r   *src.height)/mpisize) * in_bytes;
      int gylast_bytes  = (((r+1)*src.height)/mpisize) * in_bytes;
      displs   [r] = gyfirst_bytes;
      sndcounts[r] = gylast_bytes - gyfirst_bytes;
    }
  }
  else
  {
    // ces pointeurs seront ignorés par MPI_Scatterv pour tout processus !=0 (ie !=root)
    sndcounts = NULL;
    displs = NULL;
  }

  MPI_Scatterv(
      src.buf, sndcounts, displs, MPI_BYTE, // ces arguments sont ignorés pour tout processus !=0 (ie !=root)
      dst.buf, dst.sz_in_bytes, MPI_BYTE,
      /*root=*/0, MPI_COMM_WORLD
      );

  if (mpirank == 0)
  {
    free(sndcounts);
    free(displs);
  }

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

