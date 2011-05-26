#include "genpix.h"

//#define DEBUG
#define INITIAL_POP 200
#define POP_SELECT  0.33

// globals
t_image* base_image;
t_image* best_image;
t_image** population;
const int TO_SELECT = INITIAL_POP * POP_SELECT;

float image_fitness(t_image* img) {
  float fitness = 0;
  for(int x = 0; x < img->width; x++) {
    for(int y = 0; y < img->height; y++) {
      float delta_r = base_image->pix[x][y].r - img->pix[x][y].r;
      float delta_g = base_image->pix[x][y].g - img->pix[x][y].g;
      float delta_b = base_image->pix[x][y].b - img->pix[x][y].b;

      fitness += delta_r * delta_r + delta_g * delta_g + delta_b * delta_b;
    }
  }
  return fitness;
}

void init(char* image_path) {
  srand(time(0));
  int image_tex = texture_load(image_path);
  if(!image_tex) {
    fprintf(stderr, "texture load failed\n");
    exit(1);
  }

  // get the target image
  base_image = image_from_tex(image_tex);

  // create initial population
  fprintf(stderr, "seeding initial population...");
  population = s_malloc(sizeof(t_image*) * INITIAL_POP);
  for(int i = 0; i < INITIAL_POP; i++) {
    population[i] = random_image(base_image->width, base_image->height);
  }
  fprintf(stderr, "done\n");
}

void display() {
  // SELECTION
  t_image** selected = s_malloc(sizeof(t_image*) * TO_SELECT);
  // get best INITIAL_POP * POP_SELECT, draw best
  // sort population by fitness

  
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0, base_image->width, base_image->height, 0, 0, 1);
  glMatrixMode(GL_MODELVIEW);
  glBegin(GL_POINTS);
/*
  for(int i = 0; i < base_image; i++) {
    for(int j = 0; j < t_height; j++) {
      glColor3f(, rand() / (float)RAND_MAX, rand() / (float)RAND_MAX);
      glVertex2f(i + 0.5f, j + 0.5f);
    }
  }
*/
  glEnd();

  glutSwapBuffers();
}

int main(int argc, char* argv[]) {
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);

  glutInitWindowSize(256, 256);
  glutCreateWindow("exp");

  init("salad.jpg");

  glutDisplayFunc(display);
  glDisable(GL_DEPTH_TEST);
  glutMainLoop();
}
