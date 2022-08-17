#ifndef AI_H
#define AI_H

#include "grid.h"

struct ai {
  float a;
  float b;
  float c;
  float d;
  float e;
  float f;
  float g;
  int best_x;
  int best_rot;
  int pending_answer;
};

struct simple_ai {
  float a;
  float b;
  float c;
  float d;
  float e;
  float f;
  float g;
  int best_x;
  int best_rot;
};

int ai_init_worker(void);

struct ai *ai_new(
  float a, float b, float c, float d, float e, float f, float g
);
void ai_delete(struct ai *ai);
void ai_init(
  struct ai *ai, float a, float b, float c, float d, float e, float f, float g
);
void ai_run(struct ai *ai, struct dense_grid *dg);
int ai_run_async(struct ai *ai, struct dense_grid *dg);
void ai_adjust_position(struct ai *ai, struct dense_grid *dg);
void ai_adjust_position_virtual(struct ai *ai, struct dense_grid *dg);

struct simple_ai *simple_ai_new(
  float a, float b, float c, float d, float e, float f, float g
);
void simple_ai_delete(struct simple_ai *sai);
void simple_ai_init(
  struct simple_ai *sai, float a, float b, float c, float d, float e, float f,
  float g
);
void simple_ai_run(struct simple_ai *sai, struct dense_grid *dg);
void simple_ai_adjust_position(struct simple_ai *sai, struct dense_grid *dg);
void simple_ai_adjust_position_virtual(
  struct simple_ai *sai, struct dense_grid *dg
);

int benchmark_ais(
  unsigned int seed, float a, float b, float c, float d, float e, float f,
  float g
);

#endif