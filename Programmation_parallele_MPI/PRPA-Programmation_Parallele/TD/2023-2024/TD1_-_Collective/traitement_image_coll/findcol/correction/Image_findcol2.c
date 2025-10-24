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
  if (argc < 4)
  {
    fprintf(stderr, "Usage: %s [INPUT.jpg] [red] [green] [blue] \n", argv[0]);
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

  strcpy(PathName, argv[1]);


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
  }

  /*******/
  /* Recherche du ou des pixels */
  /*******/
  // Chaque processus va travailler sur SA PROPRE partie d'image
  int loc_nb_pix_found=0;

  // la liste des ids globaux trouvés
  int nb_max = 2;
  int *lpix = (int*)malloc(nb_max*sizeof(int));

  for (int y = 0; y < dst.height; y++)
  {
    for (int x = 0; x < dst.width; x++)
    {
      // le numéro id est LOCAL au processus et à son image associée
      int id = ((y * dst.width) + x);
      // pixel pointe vers la zone mémoire du pixel numéro id de dst
      unsigned char *pixel = dst.buf + id * N_COMPONENT;

      // le pixel est-il de la bonne couleur ?
      if (pixel[0] == rgb[0] &&
          pixel[1] == rgb[1] &&
          pixel[2] == rgb[2]) 
      {
        // L'identifiant global, le pixel a pour coordonnées globales (x, glob_yfirst+y)
        int id_glob = (((glob_yfirst+y) * src.width) + x);

        // On réalloue la liste si nécessaire
        if (loc_nb_pix_found >= nb_max)
        {
          nb_max = int(1.5*nb_max);
          lpix = (int*)realloc(lpix, nb_max*sizeof(int));
        }

        lpix[loc_nb_pix_found] = id_glob;
        loc_nb_pix_found++;
      }
    }
  }
  printf("P%d, nb de pixels trouvés dans ma sous-image : %d\n", 
      mpirank, loc_nb_pix_found); 

  /*******/
  /* End */
  /*******/
  // TODO : le processus 0 va d'abord récupèrer le nb de pixels trouvés par processus
  int *npix_per_proc = NULL;
  int *glob_lpix = NULL;
  int glob_nb_pix = 0;

  if (mpirank == 0)
  {
    npix_per_proc = (int*)malloc(mpisize*sizeof(int));
  }

  MPI_Gather(
      &loc_nb_pix_found, 1, MPI_INT,
      npix_per_proc, 1, MPI_INT,
      0, MPI_COMM_WORLD);

  if (mpirank == 0)
  {
    // Calcul des déplacements
    glob_nb_pix=0;
    for(int r=0 ; r<mpisize ; r++)
    {
      displs[r]=glob_nb_pix;
      glob_nb_pix += npix_per_proc[r];
    }
    glob_lpix = (int*)malloc(glob_nb_pix*sizeof(int));
  }

  MPI_Gatherv(
      lpix, loc_nb_pix_found, MPI_INT,
      glob_lpix, npix_per_proc, displs, MPI_INT,
      0, MPI_COMM_WORLD);

  if (mpirank == 0)
  {
    // TODO : écrire la liste des ids globaux et des coordonnées globales
    FILE *fd = fopen("xy_pix.txt", "w");
    for(int ipix=0 ; ipix<glob_nb_pix ; ipix++)
    {
      int id_glob = glob_lpix[ipix];
      int x_glob = id_glob % src.width;
      int y_glob = id_glob / src.width;
      fprintf(fd, "%d %d %d\n", id_glob, x_glob, y_glob);
    }
    fclose(fd);

    free(npix_per_proc);
    free(displs);
    free(glob_lpix);
  }

  free_image(&src);
  free_image(&dst);
  free(lpix);

  // Cleanup !
  MPI_Finalize();
  FreeImage_DeInitialise(); // A NE PAS RETIRER

  return 0;
}

