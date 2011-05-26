#include "image.h"

t_image* random_image(int width, int height) {
  t_image* img = s_malloc(sizeof(t_image));
  img->width = width; 
  img->height = height;
  img->pix = s_malloc(sizeof(t_rgb*) * width);
  for(int x = 0; x < width; x++) {
    img->pix[x] = s_malloc(sizeof(t_rgb) * height);
    for(int y = 0; y < height; y++) {
      img->pix[x][y].r = rand() / (float)RAND_MAX;
      img->pix[x][y].g = rand() / (float)RAND_MAX;
      img->pix[x][y].b = rand() / (float)RAND_MAX;
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
      img->pix[x][y].r = buffer[offset] / 255.0;
      img->pix[x][y].g = buffer[offset+1] / 255.0;
      img->pix[x][y].b = buffer[offset+2] / 255.0;
      /*
      printf("pixel at (%d,%d) (%d,%d,%d) (%.3f,%.3f,%.3ff)\n", x, y, 
        (int)img->pix[x][y].r*255, (int)img->pix[x][y].g*255, 
        (int)img->pix[x][y].b*255, img->pix[x][y].r, img->pix[x][y].g, 
        img->pix[x][y].b);
      */
    }
  }
  free(buffer);
  glBindTexture(GL_TEXTURE_2D, 0);
  return img;
}

void free_image(t_image* img) {
  for(int x = 0; x < img->width; x++)
    free(img->pix[x]);
  free(img->pix);
  free(img);
}
