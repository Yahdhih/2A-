#ifndef __IMAGE_H
#define __IMAGE_H


struct image_t
{
  // Les paramètres de l'image
  int width  ;
  int height ;
  int pitch  ;
  int bpp    ;

  // Les pixels de l'image
  // les lignes sont stockées les unes après les autres de manière contigüe 
  // (pas de saut en mémoire entre chaque ligne)
  unsigned int sz_in_bytes;
  unsigned char* buf; 
};

// (IN)  : img_file
// (OUT) : img_out
extern void read_image_from_file(char *img_file, image_t* img_out);

// (IN) :  img_in
// (IN) :  img_file
// (IN) :  suffix
extern void write_image_into_file(image_t* img_in, char *img_file, const char* suffix);

// (IN)  : img_inout->width, height, ...
// (OUT) : img_inout->sz_in_bytes, buf
extern void allocate_image_from_parameters(image_t *img_inout);

// (IN)  : img_inout
// (OUT) : img_inout
extern void free_image(image_t* img_inout);


#endif

