#ifndef TP_KERNEL_H
#define TP_KERNEL_H

#include <math.h>
#include "image.h"
#include "basics.h"

void saturates_red_component(int id, image_t *dst, const image_t *src)
{
  dst->buf[id * N_COMPONENT + 0] = FULL;
  dst->buf[id * N_COMPONENT + 1] = src->buf[id * N_COMPONENT + 1];
  dst->buf[id * N_COMPONENT + 2] = src->buf[id * N_COMPONENT + 2];
}

void saturates_green_component(int id, image_t *dst, const image_t *src)
{
  dst->buf[id * N_COMPONENT + 0] = src->buf[id * N_COMPONENT + 0];
  dst->buf[id * N_COMPONENT + 1] = FULL;
  dst->buf[id * N_COMPONENT + 2] = src->buf[id * N_COMPONENT + 2];
}

void saturates_blue_component(int id, image_t *dst, const image_t *src)
{
  dst->buf[id * N_COMPONENT + 0] = src->buf[id * N_COMPONENT + 0];
  dst->buf[id * N_COMPONENT + 1] = src->buf[id * N_COMPONENT + 1];
  dst->buf[id * N_COMPONENT + 2] = FULL;
}

void horizontal_symetry(int id, image_t *dst,
    const image_t *src)
{
  int size = src->width*src->height;
  dst->buf[id * N_COMPONENT + 0] = src->buf[(size - id) * N_COMPONENT + 0];
  dst->buf[id * N_COMPONENT + 1] = src->buf[(size - id) * N_COMPONENT + 1];
  dst->buf[id * N_COMPONENT + 2] = src->buf[(size - id) * N_COMPONENT + 2];
}

void blur(int id, image_t *dst, const image_t *src)
{
  const int height = src->height;
  const int width  = src->width;
  int i = id % width;
  int j = id / width;

  unsigned char mean_x = src->buf[id * N_COMPONENT + 0];
  unsigned char mean_y = src->buf[id * N_COMPONENT + 1];
  unsigned char mean_z = src->buf[id * N_COMPONENT + 2];
  int cptr = 1;

  if ((i-1) >= 0)
  {
    mean_x += src->buf[(id - 1) * N_COMPONENT + 0];
    mean_y += src->buf[(id - 1) * N_COMPONENT + 1];
    mean_z += src->buf[(id - 1) * N_COMPONENT + 2];
    cptr++;
  }

  if ((i+1)<width)
  {
    mean_x += src->buf[(id + 1) * N_COMPONENT + 0];
    mean_y += src->buf[(id + 1) * N_COMPONENT + 1];
    mean_z += src->buf[(id + 1) * N_COMPONENT + 2];
    cptr++;
  }

  if ((j-1) >= 0)
  {
    mean_x += src->buf[(id - width) * N_COMPONENT + 0];
    mean_y += src->buf[(id - width) * N_COMPONENT + 1];
    mean_z += src->buf[(id - width) * N_COMPONENT + 2];
    cptr++;
  }

  if ((j+1)<height)
  {
    mean_x += src->buf[(id + width) * N_COMPONENT + 0];
    mean_y += src->buf[(id + width) * N_COMPONENT + 1];
    mean_z += src->buf[(id + width) * N_COMPONENT + 2];
    cptr++;
  }

  dst->buf[id * N_COMPONENT + 0] = mean_x / (unsigned char)cptr;
  dst->buf[id * N_COMPONENT + 1] = mean_y / (unsigned char)cptr;
  dst->buf[id * N_COMPONENT + 2] = mean_z / (unsigned char)cptr;
}

unsigned char gray_formula(const unsigned char *pixel)
{
  return  (unsigned char) (  (float)pixel[0] * 0.299f
      + (float)pixel[1] * 0.587f
      + (float)pixel[2] * 0.114f
      );
}

void rgb_to_gray(int id, image_t *dst, const image_t *src)
{
  unsigned char rgb = gray_formula(src->buf + id * N_COMPONENT);

  dst->buf[id * N_COMPONENT + 0] = rgb;
  dst->buf[id * N_COMPONENT + 1] = rgb;
  dst->buf[id * N_COMPONENT + 2] = rgb;
}

void sobel(int id, image_t *dst, const image_t *src)
{
  const int height = src->height;
  const int width  = src->width;

  if (id % width != 0 && (id + 1) % width != 0
      && (id / width) % height != 0 && ((id + 1) / width) % height != 0)
  {
    // Init Gx and Gy
    unsigned char gx_x = 0;
    unsigned char gx_y = 0;
    unsigned char gx_z = 0;

    unsigned char gy_x = 0;
    unsigned char gy_y = 0;
    unsigned char gy_z = 0;

    /*********************/
    /* First convolution */
    /*********************/

    // Center column
    gx_x -= 2 * src->buf[(id - 1) * N_COMPONENT + 0];
    gx_y -= 2 * src->buf[(id - 1) * N_COMPONENT + 1];
    gx_z -= 2 * src->buf[(id - 1) * N_COMPONENT + 2];

    gx_x += 2 * src->buf[(id + 1) * N_COMPONENT + 0];
    gx_y += 2 * src->buf[(id + 1) * N_COMPONENT + 1];
    gx_z += 2 * src->buf[(id + 1) * N_COMPONENT + 2];

    // Left column
    gx_x -= src->buf[(id - width - 1) * N_COMPONENT + 0];
    gx_y -= src->buf[(id - width - 1) * N_COMPONENT + 1];
    gx_z -= src->buf[(id - width - 1) * N_COMPONENT + 2];

    gx_x += src->buf[(id - width + 1) * N_COMPONENT + 0];
    gx_y += src->buf[(id - width + 1) * N_COMPONENT + 1];
    gx_z += src->buf[(id - width + 1) * N_COMPONENT + 2];

    // Right column
    gx_x -= src->buf[(id + width - 1) * N_COMPONENT + 0];
    gx_y -= src->buf[(id + width - 1) * N_COMPONENT + 1];
    gx_z -= src->buf[(id + width - 1) * N_COMPONENT + 2];

    gx_x += src->buf[(id + width + 1) * N_COMPONENT + 0];
    gx_y += src->buf[(id + width + 1) * N_COMPONENT + 1];
    gx_z += src->buf[(id + width + 1) * N_COMPONENT + 2];

    /**********************/
    /* Second convolution */
    /**********************/

    // Line below
    gy_x += src->buf[(id - width - 1) * N_COMPONENT + 0];
    gy_y += src->buf[(id - width - 1) * N_COMPONENT + 1];
    gy_z += src->buf[(id - width - 1) * N_COMPONENT + 2];

    gy_x += 2 * src->buf[(id - width) * N_COMPONENT + 0];
    gy_y += 2 * src->buf[(id - width) * N_COMPONENT + 1];
    gy_z += 2 * src->buf[(id - width) * N_COMPONENT + 2];

    gy_x += src->buf[(id - width + 1) * N_COMPONENT + 0];
    gy_y += src->buf[(id - width + 1) * N_COMPONENT + 1];
    gy_z += src->buf[(id - width + 1) * N_COMPONENT + 2];

    // Line above
    gy_x -= src->buf[(id + width - 1) * N_COMPONENT + 0];
    gy_y -= src->buf[(id + width - 1) * N_COMPONENT + 1];
    gy_z -= src->buf[(id + width - 1) * N_COMPONENT + 2];

    gy_x -= 2 * src->buf[(id + width) * N_COMPONENT + 0];
    gy_y -= 2 * src->buf[(id + width) * N_COMPONENT + 1];
    gy_z -= 2 * src->buf[(id + width) * N_COMPONENT + 2];

    gy_x -= src->buf[(id + width + 1) * N_COMPONENT + 0];
    gy_y -= src->buf[(id + width + 1) * N_COMPONENT + 1];
    gy_z -= src->buf[(id + width + 1) * N_COMPONENT + 2];

    // Update
    dst->buf[id * N_COMPONENT + 0] = (unsigned char)sqrt((float)(gx_x * gx_x + gy_x * gy_x));
    dst->buf[id * N_COMPONENT + 1] = (unsigned char)sqrt((float)(gx_y * gx_y + gy_y * gy_y));
    dst->buf[id * N_COMPONENT + 2] = (unsigned char)sqrt((float)(gx_z * gx_z + gy_z * gy_z));
  }
}

void slide_effect(int id, image_t *dst, const image_t *src,
                             const int c)
{
  dst->buf[id * N_COMPONENT + 0] = (src->buf[id * N_COMPONENT + 0]<c ? 0 : src->buf[id * N_COMPONENT + 0] - c);
  dst->buf[id * N_COMPONENT + 1] = (src->buf[id * N_COMPONENT + 1]<c ? 0 : src->buf[id * N_COMPONENT + 1] - c);
  dst->buf[id * N_COMPONENT + 2] = (src->buf[id * N_COMPONENT + 2]<c ? 0 : src->buf[id * N_COMPONENT + 2] - c);
}

void keep_red_component(int id, image_t *dst, const image_t *src)
{
  dst->buf[id * N_COMPONENT + 0] = src->buf[id * N_COMPONENT + 0];
  dst->buf[id * N_COMPONENT + 1] = 0;
  dst->buf[id * N_COMPONENT + 2] = 0;
}

void keep_green_component(int id, image_t *dst, const image_t *src)
{
  dst->buf[id * N_COMPONENT + 0] = 0;
  dst->buf[id * N_COMPONENT + 1] = src->buf[id * N_COMPONENT + 1];
  dst->buf[id * N_COMPONENT + 2] = 0;
}

void keep_blue_component(int id, image_t *dst, const image_t *src)
{
  dst->buf[id * N_COMPONENT + 0] = 0;
  dst->buf[id * N_COMPONENT + 1] = 0;
  dst->buf[id * N_COMPONENT + 2] = src->buf[id * N_COMPONENT + 2];
}

void opposite_components(int id, image_t *dst, const image_t *src)
{
  dst->buf[id * N_COMPONENT + 0] = (unsigned char)255 - src->buf[id * N_COMPONENT + 0];
  dst->buf[id * N_COMPONENT + 1] = (unsigned char)255 - src->buf[id * N_COMPONENT + 1];
  dst->buf[id * N_COMPONENT + 2] = (unsigned char)255 - src->buf[id * N_COMPONENT + 2];
}

void insert_image(int id_dst, image_t *dst, 
    int id_src, const image_t *src)
{
  dst->buf[id_dst * N_COMPONENT + 0] = src->buf[id_src * N_COMPONENT + 0];
  dst->buf[id_dst * N_COMPONENT + 1] = src->buf[id_src * N_COMPONENT + 1];
  dst->buf[id_dst * N_COMPONENT + 2] = src->buf[id_src * N_COMPONENT + 2];
}

#endif

