#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>

#include "FreeImage.h"
#include "kernel.h"

int main (int argc, char **argv)
{
  MPI_Init(&argc, &argv);

  // récupérer le nombre de processus et le rang du processus
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

  // seul le processus 0 lit le fichier source d'image
  if (mpirank == 0)
  {
    // Après cet appel, src est rempli
    read_image_from_file(PathName, &src);

    // vérification que src.height est un multiple de mpisize
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
  /* Distribution des sous-parties de l'image src à dst */
  /**************************/
  MPI_Scatter(
      src.buf, dst.sz_in_bytes, MPI_BYTE, // ces arguments sont ignorés pour tout processus !=0 (ie !=root)
      dst.buf, dst.sz_in_bytes, MPI_BYTE,
      /*root=*/0, MPI_COMM_WORLD
      );

  /*********/
  /* Begin histogram */
  /*********/

  // Chaque processus va compter le nb de pixels gris dans chaque intervalle sur SA PROPRE partie d'image
#define NINTERV 8
  unsigned char ngray_per_interv = 256/NINTERV; // Nb de valeurs grises par intervalle
  int loc_histo[NINTERV] = {0, 0, 0, 0, 0, 0, 0, 0};

  for (int y = 0; y < dst.height; y++)
  {
    for (int x = 0; x < dst.width; x++)
    {
      int id = ((y * dst.width) + x);
      unsigned char *pixel = dst.buf + id * N_COMPONENT;

      // Récupération de la velru grise à partir du pixel
      unsigned char gray = gray_formula(pixel);

      // On détermine l'intervalle de l'histogramme
      unsigned char iinterv = gray/ngray_per_interv;

      // On incrémente dans un tableau local au processus
      loc_histo[iinterv]++;
    }
  }

  int root = mpisize-1; // TODO : Le processus dont le rang est le plus élevé affichera les stats


  // Histogrammes
  // NINTERV sommes indépendantes avec le même appel à MPI_Reduce
  // seul le processus root va récupérer les NINTERV sommes globales dans glob_histo
  int glob_histo[NINTERV];

  MPI_Reduce(loc_histo, glob_histo, NINTERV, MPI_INT, MPI_SUM, root, MPI_COMM_WORLD);

  if (mpirank == root) {
    // Bonne réponse pour zelda.jpg : [ 91955 676746 412169 326708 228839 167173 132742 37268 ]
    printf("Histo : [");
    for(int iinterv=0 ; iinterv<NINTERV ; iinterv++) {
      printf(" %d", glob_histo[iinterv]);
    }
    printf(" ]\n");
    // Normalisation
    float inv_npixels = (float)1/(float)(src.width*src.height); // TODO : on normalise bien par rapport à la taille de l'image d'origine (src) et non sur la taille de la sous-image du processus (dst) 
    FILE *fd = fopen("HistoNormlized.txt", "w");
    for(int iinterv=0 ; iinterv<NINTERV ; iinterv++) {
      fprintf(fd, "%f\n", glob_histo[iinterv]*inv_npixels);
    }
    fclose(fd);
  }


  /*******/
  /* End */
  /*******/
  // Sauvegarde de l'image dans un fichier avec un suffixe avec le rang MPI
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

