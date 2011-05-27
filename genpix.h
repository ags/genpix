#ifndef GENPIX_H
#define GENPIX_H

#include <time.h>
#include "image.h"
#include "util.h"

double image_fitness(t_image* img);

void flip_random_bit(int* n);

void one_point_crossover(int p_a, int p_b, int* children);

#endif
