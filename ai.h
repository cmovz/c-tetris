#ifndef AI_H
#define AI_H

#include "grid.h"

struct possible_fit2 {
  struct dense_grid dg;
  float fitness;
};

struct possible_fit1 {
  struct dense_grid dg;
  struct possible_fit2 pfs2[190];
  int pfs2_len;
  float fitness;
};

struct possible_fit {
  struct dense_grid dg;
  struct possible_fit1 pfs1[190];
  int pfs1_len;
  float fitness;
};

struct ai {
  struct possible_fit pfs[4 * 10];
  float a;
  float b;
  float c;
  float d;
  float e;
  int best_x;
  int best_rot;
  int pfs_idx;
};

struct ai *ai_new(float a, float b, float c, float d, float e);
void ai_delete(struct ai *ai);

void ai_init(struct ai *ai, float a, float b, float c, float d, float e);
void ai_run(struct ai *ai, struct dense_grid *dg);
void ai_adjust_position(struct ai *ai, struct dense_grid *dg);

#endif