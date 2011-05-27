#include "genpix.h"
#include <math.h>

//#define DEBUG
#define WINDOW_TITLE       "genpix"
#define INIT_POP_SIZE      200
#define POP_SELECT_FACTOR  0.33
#define CROSSOVER_RATE     0.75
#define MUTATION_RATE      0.25
#define MUT_MIN            -5
#define MUT_MAX            5
#define N_CHILDREN         8
#define N_PARENTS          2

// globals
t_image* base_image;
t_image* best_image;
t_image** population;
t_image** new_pop;
static int population_size;
static int pop_select;
static double* fitness;
static int done = 0;
static int generations = 0;

double image_fitness(t_image* img) {
  double fitness = 0;
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
  double f_a = image_fitness(i_a);
  double f_b = image_fitness(i_b); 
  return (f_a > f_b) - (f_a < f_b);
}

void mutate_rgb(t_rgb* rgb) {
  int r;
  r = rand() % (MUT_MAX - MUT_MIN + 1) + MUT_MIN;
  rgb->r = fmin(255, fmax(0, rgb->r + r));
  r = rand() % (MUT_MAX - MUT_MIN + 1) + MUT_MIN;
  rgb->g = fmin(255, fmax(0, rgb->g + r));
  r = rand() % (MUT_MAX - MUT_MIN + 1) + MUT_MIN;
  rgb->b = fmin(255, fmax(0, rgb->b + r));
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

  population_size = INIT_POP_SIZE;
  pop_select = population_size * POP_SELECT_FACTOR;
  fitness = s_malloc(sizeof(double) * population_size);

  // create initial population
  fprintf(stderr, "seeding initial population...");
  population = calloc(sizeof(t_image*), population_size);
  new_pop = calloc(sizeof(t_image*), population_size);

  double fitness_sum = 0;
  for(int i = 0; i < population_size; i++) {
    population[i] = random_image(base_image->width, base_image->height);
    fitness[i] = image_fitness(population[i]);
    fitness_sum += fitness[i];
  }
  // normalise fitness
  for(int i = 0; i < population_size; i++) {
    fitness[i] /= fitness_sum;
  }
  best_image = population[0];
  fprintf(stderr, "done\n");
}

void rgb_crossover(t_rgb* p_a, t_rgb* p_b, t_rgb* child) {
  int r;
  r = rand() / (float)RAND_MAX;
  child->r = (r) ? p_a->r : p_b->r;
  r = rand() / (float)RAND_MAX;
  child->g = (r) ? p_a->g : p_b->g;
  r = rand() / (float)RAND_MAX;
  child->b = (r) ? p_a->b : p_b->b;
}

void display() {
  if(!done && image_match(best_image, base_image))
    done = 1;
  if(!done) {// && generations < 10000) {
    float r, accum_fitness = 0;
    fprintf(stderr, "evolving...");
    // sort population by fitness
    qsort(population, population_size, sizeof(t_image*), image_fitness_cmp);
    // current best is start of sorted list
    best_image = population[0];
    // always add the best solution to survivors
    new_pop[0] = image_copy(best_image);
    // get more survivors using roulette wheel selection
    for(int i = 1; i < pop_select; i++) {
      r = rand() / (float)RAND_MAX;
      // use old pop here
      for(int j = 0; j < population_size; j++) {
        accum_fitness += fitness[j];
        if(accum_fitness >= r) {
          new_pop[i] = image_copy(population[j]);
          break;
        }
      }
      accum_fitness = 0;
    }
    // get a normalised fitness distribution for the survivors
    double survivor_fit[pop_select];
    double survivor_sum_fit = 0;
    for(int i = 0; i < pop_select; i++) {
      survivor_fit[i] = image_fitness(new_pop[i]);
      survivor_sum_fit += survivor_fit[i];
    }
    for(int i = 0; i < pop_select; i++) {
      survivor_fit[i] /= survivor_sum_fit;
    }

    // fill rest of population with children
    int i = pop_select;
    while(i < population_size) {
      // 2 random parents from survivors
      t_image* parents[N_PARENTS] = {0};
      for(int j = 0; j < N_PARENTS; j++) {
        r = rand() / (float)RAND_MAX;
        for(int k = 0; k < pop_select; k++) {
          accum_fitness += survivor_fit[k];
          if(accum_fitness >= r) {
            parents[j] = new_pop[k];
            break;
          }
        }
        accum_fitness = 0;
      }

      t_image* children[N_CHILDREN] = {0};
      int p = 0;
      for(int c = 0; c < N_CHILDREN; c++) {
        children[c] = image_copy(parents[p]);
        /* can get away with this instead of mod as parents is 2
          if we were to use more, have to change it */
        p = !p;
      }

      for(int x = 0; x < base_image->width; x++) {
        for(int y = 0; y < base_image->height; y++) {
          // crossover
          int p = 0;
          for(int c = 0; c < N_CHILDREN; c++) {
            //rgb_crossover(&parents[0]->pix[x][y], &parents[1]->pix[x][y], 
            //              &children[c]->pix[x][y]);
            //printf("%d %d %d %d\n", c, c & (1 << 0), c & (1 << 1), c & (1 << 2));
            // use 3 bits of c to get the 8 combinations for crossover
            p = (c & (1 << 0)) ? 1 : 0;
            children[c]->pix[x][y].r = parents[p]->pix[x][y].r;
            p = (c & (1 << 1)) ? 1 : 0;
            children[c]->pix[x][y].g = parents[p]->pix[x][y].g;
            p = (c & (1 << 2)) ? 1 : 0;
            children[c]->pix[x][y].b = parents[p]->pix[x][y].b;
          }

          // mutation
          for(int c = 0; c < N_CHILDREN; c++) {
            r = rand() / (float)RAND_MAX;
            if(r <= MUTATION_RATE)
              mutate_rgb(&children[c]->pix[x][y]);
          }
        }
      }
      // add to new pop
      for(int c = 0; c < N_CHILDREN && i < population_size; c++) {
        new_pop[i++] = children[c];
      }
    }

    // deallocate last generation
    for(int i = 0; i < population_size; i++) {
      free_image(population[i]);
      population[i] = NULL;
    }

    // set up new population 
    t_image** t_pop = population;
    population = new_pop;
    new_pop = t_pop;

    // re-calculate fitness
    double fitness_sum = 0;
    for(int i = 0; i < population_size; i++) {
      fitness[i] = image_fitness(population[i]);
      fitness_sum += fitness[i];
    }
    // normalise fitness
    for(int i = 0; i < population_size; i++) {
      fitness[i] /= fitness_sum;
    }

    qsort(population, population_size, sizeof(t_image*), image_fitness_cmp);
    // current best is start of sorted list
    best_image = population[0];
    generations++;
    fprintf(stderr, "done (gen %d)\n", generations);
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

void idle() {
  glutPostRedisplay();
}

int main(int argc, char* argv[]) {
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);

  glutInitWindowSize(58, 54);
  //glutInitWindowSize(10, 10);
  glutCreateWindow(WINDOW_TITLE);
  glutDisplayFunc(display);
  glutIdleFunc(idle);
  glDisable(GL_DEPTH_TEST);

  init("me.jpg");
  //init("ten.png");

  glutMainLoop();
}
