#ifndef UTIL_H
#define UTIL_H

#include <stdlib.h>
#include <stdio.h>

#ifdef WIN32
  #include <windows.h>
#endif

#ifdef __APPLE__
  #include <OpenGL/gl.h>
  #include <OpenGL/glu.h>
  #include <GLUT/glut.h>
#else
  #include <GL/gl.h>
  #include <GL/glu.h>
  #include <GL/glut.h>
#endif

static const float F_RAND_MAX = (float)RAND_MAX;

void* s_malloc(unsigned int bytes);

float rand_float();

#endif
