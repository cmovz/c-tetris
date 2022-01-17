#ifndef CORE_H
#define CORE_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define FATAL {\
  fprintf(stderr, "FATAL ERROR %s: %d\n", __FILE__, __LINE__); \
  perror("Error is"); \
  exit(-1); \
}

extern uint64_t now;

#endif