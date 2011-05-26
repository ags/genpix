#include "genpix.h"

//#define DEBUG
#define POP_SIZE           200
#define POP_SELECT_FACTOR  0.33

// globals
t_image* base_image;
t_image* best_image;
t_image** population;
const int POP_SELECT = POP_SIZE * POP_SELECT_FACTOR;

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

int image_fitness_cmp(const void* a, const void* b) {
  t_image* i_a = *(t_image**)a;
  t_image* i_b = *(t_image**)b;
  float f_a = image_fitness(i_a);
  float f_b = image_fitness(i_b); 
  return (f_a > f_b) - (f_a < f_b);
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
  population = s_malloc(sizeof(t_image*) * POP_SIZE);
  for(int i = 0; i < POP_SIZE; i++) {
    population[i] = random_image(base_image->width, base_image->height);
  }
  fprintf(stderr, "done\n");
}

void display() {
  if(!done) {
    // SELECTION
    // sort population by fitness
    qsort(population, POP_SIZE, sizeof(t_image*), image_fitness_cmp);
    // current best is start of sorted list
    best_image = population[0];
    for(int i = 0; i < POP_SELECT; i++) {
      fprintf(stderr, "img %d fitness %.3f\n", i, 
        image_fitness(population[i]));
    }
  }

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0, base_image->width, base_image->height, 0, 0, 1);
  glMatrixMode(GL_MODELVIEW);
  glBegin(GL_POINTS);
  for(int x = 0; x < best_image->width; x++) {
    for(int y = 0; y < best_image->height; y++) {
      glColor3f(best_image->pix[x][y].r, best_image->pix[x][y].g,
                  best_image->pix[x][y].b);
      glVertex2f(x + 0.5f, y + 0.5f);
    }
  }
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
