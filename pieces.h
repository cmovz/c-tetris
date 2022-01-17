#ifndef PIECES_H
#define PIECES_H

#define PIECE_COUNT 7

#include "colors.h"
#include <stdint.h>

struct piece {
  uint64_t matrices;
  uint8_t rot_count;
  uint8_t color;
};

extern struct piece *pieces[PIECE_COUNT];

#endif