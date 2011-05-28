#include "image.h"

t_image* random_image(int width, int height) {
  t_image* img = s_malloc(sizeof(t_image));
  img->width = width; 
  img->height = height;
  img->pix = s_malloc(sizeof(t_rgb*) * width);
  for(int x = 0; x < width; x++) {
    img->pix[x] = s_malloc(sizeof(t_rgb) * height);
    for(int y = 0; y < height; y++) {
      img->pix[x][y].r = 0;//rand() % 256;
      img->pix[x][y].g = 0;//rand() % 256;
      img->pix[x][y].b = 0;//rand() % 256;
    }
  }
  return img;
}

t_image* image_from_tex(int image_tex) {
  t_image* img = s_malloc(sizeof(t_image));
  glBindTexture(GL_TEXTURE_2D, image_tex);

  glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &img->width);
  glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &img->height);

  GLubyte* buffer = s_malloc(img->width * img->height * 4);
  img->pix = s_malloc(sizeof(t_rgb*) * img->width);

  // read texture into buffer
  glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
  // convert to [0.0, 1.0] range floats
  for(int x = 0; x < img->width; x++) {
    img->pix[x] = s_malloc(sizeof(t_rgb) * img->height);
    for(int y = 0; y < img->height; y++) {
      int offset = ((img->width * y) + x) * 4;
      img->pix[x][img->height-1 - y].r = buffer[offset];
      img->pix[x][img->height-1 - y].g = buffer[offset+1];
      img->pix[x][img->height-1 - y].b = buffer[offset+2];
    }
  }
  free(buffer);
  glBindTexture(GL_TEXTURE_2D, 0);
  return img;
}

t_image* image_copy(t_image* img) {
  t_image* n_img = s_malloc(sizeof(t_image));
  n_img->width = img->width; 
  n_img->height = img->height;
  n_img->pix = s_malloc(sizeof(t_rgb*) * n_img->width);
  for(int x = 0; x < n_img->width; x++) {
    n_img->pix[x] = s_malloc(sizeof(t_rgb) * n_img->height);
    for(int y = 0; y < n_img->height; y++) {
      n_img->pix[x][y].r = img->pix[x][y].r;
      n_img->pix[x][y].g = img->pix[x][y].g;
      n_img->pix[x][y].b = img->pix[x][y].b;
    }
  }
  return n_img;
}

int image_match(t_image* a, t_image* b) {
  for(int x = 0; x < a->width; x++) {
    for(int y = 0; y < a->height; y++) {
      if(a->pix[x][y].r != b->pix[x][y].r)
        return 0;
      if(a->pix[x][y].g != b->pix[x][y].g)
        return 0;
      if(a->pix[x][y].b != b->pix[x][y].b)
        return 0;
    }
  } 
  return 1;
}

void free_image(t_image* img) {
  for(int x = 0; x < img->width; x++)
    free(img->pix[x]);
  free(img->pix);
  free(img);
}
