#include "genpix.h"

//#define DEBUG
#define POP_SIZE           200
#define POP_SELECT_FACTOR  0.33

// globals
const int POP_SELECT = POP_SIZE * POP_SELECT_FACTOR;

t_image* base_image;
t_image* best_image;
t_image* population[POP_SIZE];
float fitness[POP_SIZE];

int done = 0;

int image_fitness(t_image* img) {
  int fitness = 0;
  for(int x = 0; x < img->width; x++) {
    for(int y = 0; y < img->height; y++) {
      int delta_r = base_image->pix[x][y].r - img->pix[x][y].r;
      int delta_g = base_image->pix[x][y].g - img->pix[x][y].g;
      int delta_b = base_image->pix[x][y].b - img->pix[x][y].b;
      fitness += delta_r * delta_r + delta_g * delta_g + delta_b * delta_b;
    }
  }
  return fitness;
}

int image_fitness_cmp(const void* a, const void* b) {
  t_image* i_a = *(t_image**)a;
  t_image* i_b = *(t_image**)b;
  int f_a = image_fitness(i_a);
  int f_b = image_fitness(i_b); 
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
  //population = s_malloc(sizeof(t_image*) * POP_SIZE);
  int fitness_sum = 0;
  for(int i = 0; i < POP_SIZE; i++) {
    population[i] = random_image(base_image->width, base_image->height);
    fitness[i] = image_fitness(population[i]);
    fitness_sum += fitness[i];
  }
  // normalise fitness
  for(int i = 0; i < POP_SIZE; i++) {
    fitness[i] /= fitness_sum;
  }
  fprintf(stderr, "done\n");
}

void flip_random_bit(int n) {
  n ^= 1 << (rand() % 32);
}

void one_point_crossover(int p_a, int p_b, int* children) {
  int c_point = rand() % 32;
  children[0] = p_a;
  children[1] = p_b;
  // if bit is set in parent, set it in opposite child
  for(int i = c_point; i < 32; i++) {
    if(p_b & (1 << i))
      children[0] |= 1 << i;
    else
      children[0] &= ~(1 << i);

    if(p_a & (1 << i))
      children[1] |= 1 << i;
    else
      children[1] &= ~(1 << i);
  }
}

void display() {
  if(!done) {
    // SELECTION
    // sort population by fitness
    qsort(population, POP_SIZE, sizeof(t_image*), image_fitness_cmp);
    // current best is start of sorted list
    best_image = population[0];
    float accum_fitness;
    float r = rand() / (float)RAND_MAX;
    t_image* select;
    for(int i = 0; i < POP_SELECT; i++) {
      for(int j = 0; j < POP_SIZE; j++) {
        accum_fitness += fitness[j];
        if(accum_fitness >= r) {
          select = population[j];
          break;
        }
      }
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
      glColor3f(best_image->pix[x][y].r/255.0, best_image->pix[x][y].g/255.0,
                  best_image->pix[x][y].b/255.0);
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
