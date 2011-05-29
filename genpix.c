#include "genpix.h"
#include <math.h>

//#define DEBUG
#define WINDOW_TITLE       "genpix"
#define MAX_GENS           100000
#define INIT_POP_SIZE      60
#define POP_SELECT_FACTOR  0.20
#define CROSSOVER_RATE     0.95
#define MUTATION_RATE      0.01
#define MUT_MIN            -15
#define MUT_MAX            15
#define N_CHILDREN         2
#define N_PARENTS          2

static t_image* base_image = NULL;
static t_image* best_image = NULL;
static t_image** population;
static double last_best;
static int population_size;
static int pop_select;
static int done = 0;
static int generations = 0;
static int paused = 0;

double image_fitness(t_image* img) {
  double fitness = 0;
  int delta_r, delta_g, delta_b;
  for(int x = 0; x < img->width; x++) {
    for(int y = 0; y < img->height; y++) {
      delta_r = abs(base_image->pix[x][y].r - img->pix[x][y].r);
      delta_g = abs(base_image->pix[x][y].g - img->pix[x][y].g);
      delta_b = abs(base_image->pix[x][y].b - img->pix[x][y].b);
      fitness += delta_r + delta_g + delta_b;
    }
  }
  return fitness;
}

int image_fitness_cmp(const void* a, const void* b) {
  double f_a = image_fitness(*(t_image**)a);
  double f_b = image_fitness(*(t_image**)b); 
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
  /*
  rgb->r = rand() % 256;
  rgb->g = rand() % 256;
  rgb->b = rand() % 256;
  */
}

void average_rgb(int x, int y, t_image** parents, t_image* child) {
  int r_sum = 0, g_sum = 0, b_sum = 0;
  for(int i = 0; i < N_PARENTS; i++) {
    r_sum += parents[i]->pix[x][y].r;
    g_sum += parents[i]->pix[x][y].g;
    b_sum += parents[i]->pix[x][y].b;
  }
  r_sum /= N_PARENTS;
  g_sum /= N_PARENTS;
  b_sum /= N_PARENTS;
  child->pix[x][y].r = r_sum;
  child->pix[x][y].g = g_sum;
  child->pix[x][y].b = b_sum;
}

void cross_rgb(int x, int y, t_image** parents, t_image* child) {
  int r_p, r_m, delta;
  r_p = rand() % 2;
  r_m = (0.9 - 0.1) * ((float)rand() / RAND_MAX) + 0.1;
  delta = parents[0]->pix[x][y].r - parents[1]->pix[x][y].r;
  child->pix[x][y].r = parents[r_p]->pix[x][y].r + (delta * r_m);
  
  r_p = rand() % 2;
  r_m = (0.9 - 0.1) * ((float)rand() / RAND_MAX) + 0.1;
  delta = parents[0]->pix[x][y].g - parents[1]->pix[x][y].g;
  child->pix[x][y].g = parents[r_p]->pix[x][y].g + (delta * r_m);

  r_p = rand() % 2;
  r_m = (0.9 - 0.1) * ((float)rand() / RAND_MAX) + 0.1;
  delta = parents[0]->pix[x][y].b - parents[1]->pix[x][y].b;
  child->pix[x][y].b = parents[r_p]->pix[x][y].b + (delta * r_m);
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
  
  // create initial population
  fprintf(stderr, "seeding initial population...");
  population = s_malloc(population_size * sizeof(t_image*));

  for(int i = 0; i < population_size; i++) {
    population[i] = random_image(base_image->width, base_image->height);
  }
  qsort(population, population_size, sizeof(t_image*), image_fitness_cmp);
  best_image = image_copy(population[0]);
  last_best = image_fitness(population[0]);
  fprintf(stderr, "done\n");
}

void display() {
  if(!done && image_match(best_image, base_image)) {
    done = 1;
  }
  if(!paused && !done && generations < MAX_GENS) {
    float r;
    double accum_fitness = 0;
    fprintf(stderr, "evolving...");
    // current best is start of sorted list
    t_image** new_pop = s_malloc(population_size * sizeof(t_image*));
    // always add the best solution to survivors
    new_pop[0] = image_copy(best_image);
    // get more survivors using roulette wheel selection
    for(int i = 1; i < pop_select; i++) {
      r = rand_float();
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
        r = rand_float();
        for(int k = 0; k < pop_select; k++) {
          accum_fitness += survivor_fit[k];
          if(accum_fitness >= r) {
            parents[j] = new_pop[k];
            break;
          }
        }
        accum_fitness = 0;
      }

      int p = 0;
      t_image* children[N_CHILDREN] = {0};
      // seed children with parent data
      for(int c = 0; c < N_CHILDREN; c++) {
        children[c] = image_copy(parents[p]);
        /* NOTE: can get away with this instead of mod as parents is 2,
           but if we were to use more have to change it */
        p = !p;
      }

      for(int x = 0; x < base_image->width; x++) {
        for(int y = 0; y < base_image->height; y++) {
          // crossover
          for(int c = 0; c < N_CHILDREN; c++) {
            /*
            // use 3 bits of c to get the 8 combinations for crossover
            p = (c & (1 << 0)) ? 1 : 0;
            children[c]->pix[x][y].r = parents[p]->pix[x][y].r;
            p = (c & (1 << 1)) ? 1 : 0;
            children[c]->pix[x][y].g = parents[p]->pix[x][y].g;
            p = (c & (1 << 2)) ? 1 : 0;
            children[c]->pix[x][y].b = parents[p]->pix[x][y].b;
            */
            average_rgb(x, y, parents, children[c]);
          }

          // mutation
          for(int c = 0; c < N_CHILDREN; c++) {
            if(rand_float() <= MUTATION_RATE)
              mutate_rgb(&children[c]->pix[x][y]);
          }
        }
      }

      // add to new population
      for(int c = 0; c < N_CHILDREN; c++) {
        // this child is 'born'
        if(i < population_size && rand_float() <= CROSSOVER_RATE) {
          new_pop[i++] = children[c];
        // you are dead to me!
        } else {
          free_image(children[c]);
        }
      }
    }

    // deallocate last generation
    for(int i = 0; i < population_size; i++) {
      free_image(population[i]);
    }
    free(population);
    
    // set new population as current
    population = new_pop;
    // determine new best member and compare to old best
    qsort(population, population_size, sizeof(t_image*), image_fitness_cmp);
    double current_fit = image_fitness(population[0]);
    if(current_fit < last_best) {
      free_image(best_image);
      best_image = image_copy(population[0]);
      last_best = current_fit;
    }
    generations++;
    fprintf(stderr, "done | gen %d | f %.1f\n", generations, current_fit);
  }

  // draw the best image
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

void keyboardDown(unsigned char key, int x, int y) {
  switch(key) {
    case 'q':
      for(int i = 0; i < population_size; i++)
        free_image(population[i]);
      free(population);
      free_image(best_image);
      exit(0);
      break;
    case 'p':
      paused = !paused;
      break;
    default:
      break;
  }
}

int main(int argc, char* argv[]) {
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);

  //glutInitWindowSize(100, 100);
  /* TODO
   should be able to do init here, then use best->width etc as
   window size, but for some reason this doesn't work so read
   from cli for now
  */
  glutInitWindowSize(atoi(argv[2]), atoi(argv[3]));
  glutCreateWindow(WINDOW_TITLE);
  glutDisplayFunc(display);
  glutIdleFunc(idle);
  glutKeyboardFunc(keyboardDown);
  glDisable(GL_DEPTH_TEST);

  //init("tux.jpg");
  init(argv[1]);

  glutMainLoop();
}
