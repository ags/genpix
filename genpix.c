#include "genpix.h"
#include <math.h>

//#define DEBUG
#define WINDOW_TITLE       "genpix"
#define MAX_GENS           100000
#define INIT_POP_SIZE      100
#define POP_SELECT_FACTOR  0.05
#define CROSSOVER_RATE     0.85
#define MUTATION_RATE      0.05
#define MUT_MIN            -55
#define MUT_MAX            55
#define N_CHILDREN         8
#define N_PARENTS          2

// globals
static t_image* base_image = NULL;
static t_image* best_image = NULL;
static t_image** population;
static int population_size;
static int pop_select;
//static double* fitness;
static double last_best;
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
  //fitness = s_malloc(sizeof(double) * population_size);

  // create initial population
  fprintf(stderr, "seeding initial population...");
  population = malloc(population_size * sizeof(t_image*));

  for(int i = 0; i < population_size; i++) {
    population[i] = random_image(base_image->width, base_image->height);
  }
  qsort(population, population_size, sizeof(t_image*), image_fitness_cmp);
  best_image = image_copy(population[0]);
  last_best = image_fitness(population[0]);
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

/*void calculate_fitness() {
  double fitness_sum = 0;
  for(int i = 0; i < population_size; i++) {
    fitness[i] = image_fitness(population[i]);
    fitness_sum += fitness[i];
  }
  // normalise fitness
  for(int i = 0; i < population_size; i++) {
    fitness[i] /= fitness_sum;
  }
}*/

void display() {
  if(!done && image_match(best_image, base_image)) {
    done = 1;
  }
  if(!done && generations < MAX_GENS) {
    float r;
    double accum_fitness = 0;
    fprintf(stderr, "evolving...");
    // current best is start of sorted list
    t_image** new_pop = malloc(population_size * sizeof(t_image*));
    // always add the best solution to survivors
    new_pop[0] = image_copy(best_image);
    // get more survivors using roulette wheel selection
    for(int i = 1; i < pop_select; i++) {
      r = rand() / (float)RAND_MAX;
      for(int j = 0; j < population_size; j++) {
        accum_fitness += image_fitness(population[j]);
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
//            rgb_crossover(&parents[0]->pix[x][y], &parents[1]->pix[x][y], &children[c]->pix[x][y]);

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
    }
    free(population);
    population = new_pop;
    qsort(population, population_size, sizeof(t_image*), image_fitness_cmp);
    // current best is start of sorted list
    double current_fit = image_fitness(population[0]);
    if(current_fit < last_best) {
      best_image = image_copy(population[0]);
      last_best = current_fit;
    }
    generations++;
    fprintf(stderr, "done (gen %d) %f\n", generations, current_fit);
  }
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0, base_image->width, base_image->height, 0, 0, 1);
  glMatrixMode(GL_MODELVIEW);
  glBegin(GL_POINTS);
  fprintf(stderr, "drawing...");
  for(int x = 0; x < best_image->width; x++) {
    for(int y = 0; y < best_image->height; y++) {
      glColor3f(best_image->pix[x][y].r/255.0, best_image->pix[x][y].g/255.0,
                  best_image->pix[x][y].b/255.0);
      glVertex2f(x + 0.5f, y + 0.5f);
    }
  }
  glEnd();
  fprintf(stderr, "done\n");

  glutSwapBuffers();
}

void idle() {
  glutPostRedisplay();
}

int main(int argc, char* argv[]) {
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);

  //glutInitWindowSize(128, 119);
  //glutInitWindowSize(100, 100);
  glutInitWindowSize(58, 54);
  glutCreateWindow(WINDOW_TITLE);
  glutDisplayFunc(display);
  glutIdleFunc(idle);
  glDisable(GL_DEPTH_TEST);

  //init("bunny-rrr.png");
  //init("ten.png");
  init("me.jpg");
  //init(argv[1]);

  glutMainLoop();
}
