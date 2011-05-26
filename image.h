#ifndef IMAGE_H
#define IMAGE_H

#include "util.h"
#include "texture.h"

typedef struct {
  int r, g, b;
} t_rgb;

typedef struct {
  t_rgb** pix;
  int width, height;
} t_image;

t_image* image_from_tex(int image_tex);
t_image* random_image(int width, int height);
void free_image(t_image* img);

#endif
