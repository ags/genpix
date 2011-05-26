#include "util.h"

void* s_malloc(unsigned int bytes) {
  void* ptr = malloc(bytes);
  if(!ptr) {
    fprintf(stderr, "failed to allocate %d bytes\n", bytes);
    exit(EXIT_FAILURE);
  }
  return ptr;
}
