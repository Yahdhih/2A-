#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "FreeImage.h"
#include "kernel.h"

int main (int argc, char **argv)
{
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

  // Après cet appel, src est rempli
  read_image_from_file(PathName, &src);

  // Recopie des paramètres de src dans dst
  dst.width  = src.width ;
  dst.height = src.height;
  dst.pitch  = src.pitch ;
  dst.bpp    = src.bpp   ;

  // Calcul de dst.sz_in_bytes et allocation de dst.buf à partir des paramètres dst.width, dst.height, ...
  allocate_image_from_parameters(&dst);

  /**********/
  /* Kernel */
  /**********/
  for (int y = 0; y < src.height; y++)
  {
    for (int x = 0; x < src.width; x++)
    {
      int id = ((y * src.width) + x);
//      saturates_blue_component(id, &dst, &src);
//      horizontal_symetry(id, &dst, &src);
//      blur(id, &dst, &src);
//      rgb_to_gray(id, &dst, &src);
//      sobel(id, &dst, &src);
      slide_effect(id, &dst, &src, 50);
//      keep_red_component(id, &dst, &src);
//      keep_green_component(id, &dst, &src);
//      keep_blue_component(id, &dst, &src);
//      opposite_components(id, &dst, &src);
    }
  }


  /*******/
  /* End */
  /*******/
  // Sauvegarde de l'image dans un fichier avec un suffixe vide ""
  write_image_into_file(&dst, PathDest, "");

  free_image(&src);
  free_image(&dst);

  // Cleanup !
  FreeImage_DeInitialise(); // A NE PAS RETIRER

  return 0;
}

