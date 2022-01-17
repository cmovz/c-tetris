#ifndef AI_H
#define AI_H

#include "grid.h"

struct ai {
  float a;
  float b;
  float c;
  float d;
  float e;
  int best_x;
  int best_rot;
};

void ai_init(struct ai *ai, float a, float b, float c, float d, float e);
void ai_run(struct ai *ai, struct dense_grid *dg);
void ai_adjust_position(struct ai *ai, struct dense_grid *dg);

#endif