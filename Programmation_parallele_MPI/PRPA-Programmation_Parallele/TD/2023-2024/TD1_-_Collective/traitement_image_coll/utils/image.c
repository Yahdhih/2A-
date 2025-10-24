#include "image.h"
#include "basics.h"
#include "FreeImage.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/***************************************************************************/
/* Implémentation */
/***************************************************************************/

// (IN)  : bitmap
// (OUT) : img
static void get_pixels_b2i(FIBITMAP *bitmap, unsigned char *img,
    int height, int width, int pitch)
{
  //
  BYTE *bits = (BYTE*)FreeImage_GetBits(bitmap);

  //
  for (int y = 0; y < height; y++)
  {
    BYTE *pixel = (BYTE *)bits;
    for (int x = 0; x < width; x++)
    {
      int idx = ((y * width) + x) * N_COMPONENT;
      img[idx + 0] = pixel[FI_RGBA_RED];
      img[idx + 1] = pixel[FI_RGBA_GREEN];
      img[idx + 2] = pixel[FI_RGBA_BLUE];

      // next pixel
      pixel += N_COMPONENT;
    }

    // next line
    bits += pitch;
  }
}

// (OUT) : bitmap
// (IN)  : img
static void store_pixels_i2b(FIBITMAP *bitmap,
    const unsigned char *img,
    const int height, const int width,
    const int pitch)
{
  BYTE *bits = (BYTE*)FreeImage_GetBits(bitmap);

  for (int y = 0; y < height; y++)
  {
    BYTE *pixel = (BYTE *)bits;
    for (int x = 0; x < width; x++)
    {
      // Compute new pixel
      RGBQUAD newcolor;

      int idx = ((y * width) + x) * N_COMPONENT;
      newcolor.rgbRed = img[idx + 0];
      newcolor.rgbGreen = img[idx + 1];
      newcolor.rgbBlue = img[idx + 2];

      // Update pixel
      if(!FreeImage_SetPixelColor(bitmap, x, y, &newcolor))
      {
	fprintf(stderr, "(%d, %d) Fail...\n", x, y);
      }

      // next pixel
      pixel += N_COMPONENT;
    }

    // next line
    bits += pitch;
  }
}

// (IN)  : img_file
// (OUT) : img_out
void read_image_from_file(char *img_file, image_t* img_out)
{
  FREE_IMAGE_FORMAT fif_pathname;
  FIBITMAP *bitmap;

  // Load and decode a regular file
  fif_pathname = FreeImage_GetFIFFromFilename(img_file);

  bitmap = FreeImage_Load(fif_pathname, img_file, 0);

  if(!bitmap)
  {
    fprintf(stderr, "Something went wrong with the image allocation/loading !\n");
    exit(1); 
  }

  img_out->width       = FreeImage_GetWidth(bitmap);
  img_out->height      = FreeImage_GetHeight(bitmap);
  img_out->pitch       = FreeImage_GetPitch(bitmap);
  img_out->bpp         = FreeImage_GetBPP(bitmap);
  img_out->sz_in_bytes =
    sizeof(unsigned char) * N_COMPONENT * img_out->width * img_out->height;

  fprintf(stdout, "Processing Image of size %d x %d (with pitch %d , with bpp %d) \n", 
      img_out->width ,     
      img_out->height,
      img_out->pitch ,
      img_out->bpp   );


  // Allocate memories
  img_out->buf = (unsigned char *)malloc(img_out->sz_in_bytes);

  // Get pixels: bitmap => img_out->buf
  get_pixels_b2i(bitmap, img_out->buf, img_out->height, img_out->width, img_out->pitch);
}

// (IN) :  img_in
// (IN) :  img_file
// (IN) :  suffix
void write_image_into_file(image_t* img_in, char *img_file, const char* suffix)
{
  FIBITMAP *bitmap;

  bitmap = FreeImage_Allocate (img_in->width, img_in->height, img_in->bpp, 0, 0 ,0); // 0, 0, 0 = filtres sur les couleurs

  // Store pixels
  store_pixels_i2b(bitmap, 
      img_in->buf, 
      img_in->height, 
      img_in->width, 
      img_in->pitch);

  char PathDest[256];
  strcpy(PathDest, img_file);
  char PathDest_postfix [4]="m";
  char PathDest_format [5]="m";
  int length = strlen(PathDest);

  strcpy(PathDest_format, (char *) &(PathDest[length-4]));
  sprintf(PathDest_postfix, "_%s", suffix);

  PathDest[length-4]='\0';
  PathDest[length-3]='\0';
  PathDest[length-2]='\0';
  PathDest[length-1]='\0';


  if (strlen(suffix) != 0) {
    strcat(PathDest, PathDest_postfix);
  }
  strcat(PathDest, PathDest_format);

  fprintf(stderr,"PathDest = %s\n", PathDest);

  FREE_IMAGE_FORMAT fif_dest = FreeImage_GetFIFFromFilename(PathDest);

  if(FreeImage_Save(fif_dest, bitmap, PathDest, 0))
    fprintf(stderr, "Image successfully saved !\n");
  else
    fprintf(stderr, "WTF?! We can't even saved image!\n");

}

// (IN)  : img_inout
// (OUT) : img_inout
void allocate_image_from_parameters(image_t *img_inout)
{
  // Allocate memories
  img_inout->sz_in_bytes =
    sizeof(unsigned char) * N_COMPONENT * img_inout->width * img_inout->height;

  img_inout->buf = (unsigned char *)malloc(img_inout->sz_in_bytes);
}

// (IN)  : img_inout
// (OUT) : img_inout
void free_image(image_t* img_inout)
{
  if (img_inout->buf)
    free(img_inout->buf);

  img_inout->width       = 0;
  img_inout->height      = 0;
  img_inout->pitch       = 0;
  img_inout->bpp         = 0;
  img_inout->sz_in_bytes = 0;
}


