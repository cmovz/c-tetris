#include "ai.h"
#include "pieces.h"
#include <stdlib.h>
#include <stdio.h>
#include <SDL2/SDL.h>

struct minmax {
  int min;
  int max;
};

static struct minmax find_min_max_x(struct dense_grid *dg);

static int float_cmp(const void *aptr, const void *bptr)
{
  const float *a = aptr, *b = bptr;
  if (*a < *b)
    return -1;
  if (*a > *b)
    return 1;
  return 0;
}

static inline float compute_fitness_sai(
  struct simple_ai *sai, struct dense_grid *dg, int filled_rows
)
{
  return filled_rows * sai->a
    - dg->bumpiness * sai->b
    - dg->aggregate_height * sai->c
    - dg->holes * sai->d
    - dg->wells_depth * sai->e
    - dg->blocked_wells_depth * sai->f
    - dg->blocking_wells * sai->g;
}

static inline float compute_fitness(
  struct ai *ai, struct dense_grid *dg, int filled_rows
)
{
  return filled_rows * ai->a
    - dg->bumpiness * ai->b
    - dg->aggregate_height * ai->c
    - dg->holes * ai->d
    - dg->wells_depth * ai->e
    - dg->blocked_wells_depth * ai->f
    - dg->blocking_wells * ai->g;
}

static float compute_median_fitness(struct ai *ai, struct dense_grid *dg, int r)
{
  float fitnesses[190];
  int pos = 0;

  for (int p = 0; p < PIECE_COUNT; ++p) {
    struct dense_grid outer_grid = *dg;
    dense_grid_add_piece(&outer_grid, pieces[p], 6, 1);
    for (int rot = 0; rot < pieces[p]->rot_count; ++rot) {
      outer_grid.piece_rot = rot;
      struct minmax mm = find_min_max_x(&outer_grid);
      for (int x = mm.min; x < mm.max; ++x, ++pos) {
        struct dense_grid inner_grid = outer_grid;
        inner_grid.piece_x = x;

        int game_over = 0;
        while (dense_grid_move_piece(&inner_grid, 0, 1)) { 
        }
        if (!dense_grid_move_piece(&inner_grid, 0, -1))
          game_over = 1;
        
        if (game_over)
          fitnesses[pos] = -10000000.0f;
        else {
          if (r > 0) {
            int filled_rows = dense_grid_integrate_piece(&inner_grid);
            fitnesses[pos] = compute_fitness(ai, &inner_grid, filled_rows);
          }
          else
            fitnesses[pos] = compute_median_fitness(ai, &inner_grid, r + 1);
        }
      }
    }
  }

  qsort(fitnesses, pos, sizeof fitnesses[0], float_cmp);
  int median_idx0 = (pos - 1) / 2;
  int median_idx1 = pos / 2;
  return (fitnesses[median_idx0] + fitnesses[median_idx1]) / 2.0f;
}

static struct minmax find_min_max_x(struct dense_grid *dg)
{
  int original_x = dg->piece_x;
  int min_x = original_x;
  int max_x = original_x;

  do {
    --min_x;
    if (!dense_grid_move_piece(dg, -1, 0))
      break;
  } while(1);
  dense_grid_move_piece_no_check(dg, original_x - min_x, 0);
  ++min_x;

  do {
    ++max_x;
    if (!dense_grid_move_piece(dg, 1, 0))
      break;
  } while(1);
  dense_grid_move_piece_no_check(dg, original_x - max_x, 0);

  struct minmax mm = {min_x, max_x};
  return mm;
} 

static void send_keypress(int scancode)
{
  SDL_Event event;
  event.type = SDL_KEYDOWN;
  event.key.keysym.scancode = scancode;
  SDL_PushEvent(&event);
  event.type = SDL_KEYUP;
  SDL_PushEvent(&event);
}

struct ai *ai_new(float a, float b, float c, float d, float e, float f, float g)
{
  struct ai *ai = malloc(sizeof *ai);
  if (!ai)
    return NULL;

  ai_init(ai, a, b, c, d, e, f, g);
  return ai;
}

void ai_delete(struct ai *ai)
{
  free(ai);
}

void ai_init(
  struct ai *ai, float a, float b, float c, float d, float e, float f, float g
)
{
  ai->a = a;
  ai->b = b;
  ai->c = c;
  ai->d = d;
  ai->e = e;
  ai->f = f;
  ai->g = g;
  ai->best_x = 0;
  ai->best_rot = 0;
}

void ai_run(struct ai *ai, struct dense_grid *dg)
{
  struct dense_grid dgrids[40];
  struct dense_grid *chosen_grid = NULL;
  int best_x = 6;
  int best_rot = 0;
  float best_fitness = -10000000.0f;
  int pos = 0;

  for (int rot = 0; rot < dg->piece_rot_count; ++rot) {
    int original_rot = dg->piece_rot;
    dg->piece_rot = rot;
    struct minmax mm = find_min_max_x(dg);
    for (int x = mm.min; x < mm.max; ++x, ++pos) {
      dgrids[pos] = *dg;
      dgrids[pos].piece_x = x;

      int game_over = 0;
      while (dense_grid_move_piece(dgrids + pos, 0, 1)){
      }
      if (!dense_grid_move_piece(dgrids + pos, 0, -1))
        game_over = 1;
      
      if (!game_over) {
        int filled_rows = dense_grid_integrate_piece(dgrids + pos);
        float fitness = compute_fitness(ai, dgrids + pos, filled_rows);
        if (fitness > best_fitness) {
          best_fitness = fitness;
          chosen_grid = dgrids + pos;
          best_x = x;
          best_rot = rot;
        }
      }
    }
    dg->piece_rot = original_rot;    
  }

  if (chosen_grid) {
    struct dense_grid *grid_chosen_next = chosen_grid;
    int hole_count = chosen_grid->holes;
    best_fitness = compute_median_fitness(ai, dgrids, 0);
    best_x = dgrids[0].piece_x;
    best_rot = dgrids[0].piece_rot;
    for (int i = 1; i < pos; ++i) { 
      float fitness = compute_median_fitness(ai, dgrids + i, 0);
      if (fitness > best_fitness && dgrids[i].holes <= hole_count) {
        best_x = dgrids[i].piece_x;
        best_rot = dgrids[i].piece_rot;
        best_fitness = fitness;
        grid_chosen_next = dgrids + i;
      }
    }
    if (grid_chosen_next != chosen_grid)
      puts("changing choice");
    else
      puts("same choice");
  }

  ai->best_x = best_x;
  ai->best_rot = best_rot;
}


void ai_adjust_position(struct ai *ai, struct dense_grid *dg)
{
  int x = dg->piece_x;
  int rot = dg->piece_rot;
  int moved = 0;

  if (x < ai->best_x) {
    send_keypress(SDL_SCANCODE_RIGHT);
    moved = 1;
  }
  else if (x > ai->best_x) {
    send_keypress(SDL_SCANCODE_LEFT);
    moved = 1;
  }

  if (rot != ai->best_rot) {
    send_keypress(SDL_SCANCODE_UP);
    moved = 1;
  }

  if (!moved)
    send_keypress(SDL_SCANCODE_DOWN);
}

void ai_adjust_position_virtual(struct ai *ai, struct dense_grid *dg)
{
  while (dg->piece_rot != ai->best_rot) {
    if (!dense_grid_rotate_piece(dg)) {
      dense_grid_rotate_piece_backwards_no_check(dg);
      break;
    }
  }

  while (dg->piece_x < ai->best_x) {
    if (!dense_grid_move_piece(dg, 1, 0)) {
      dense_grid_move_piece_no_check(dg, -1, 0);
      break;
    }
  }

  while (dg->piece_x > ai->best_x) {
    if (!dense_grid_move_piece(dg, -1, 0)) {
      dense_grid_move_piece_no_check(dg, 1, 0);
      break;
    }
  }
}

struct simple_ai *simple_ai_new(
  float a, float b, float c, float d, float e, float f, float g
)
{
  struct simple_ai *sai = malloc(sizeof *sai);
  if (!sai)
    return NULL;
  
  simple_ai_init(sai, a, b, c, d, e, f, g);
  return sai;
}

void simple_ai_delete(struct simple_ai *sai)
{
  free(sai);
}

void simple_ai_init(
  struct simple_ai *sai, float a, float b, float c, float d, float e, float f,
  float g
)
{
  sai->a = a;
  sai->b = b;
  sai->c = c;
  sai->d = d;
  sai->e = e;
  sai->f = f;
  sai->g = g;
  sai->best_x = 0;
  sai->best_rot = 0;
}

void simple_ai_run(struct simple_ai *sai, struct dense_grid *dg)
{
  int best_rot = 0;
  int best_x = 6;
  float best_fitness = -10000000.f;

  for (int rot = 0; rot < dg->piece_rot_count; ++rot) {
    int original_rot = dg->piece_rot;
    dg->piece_rot = rot;
    struct minmax mm = find_min_max_x(dg);
    for (int x = mm.min; x < mm.max; ++x) {
      struct dense_grid grid = *dg;
      grid.piece_x = x;

      int game_over = 0;
      while (dense_grid_move_piece(&grid, 0, 1)){
      }
      if (!dense_grid_move_piece(&grid, 0, -1))
        game_over = 1;
      
      if (!game_over) {
        int filled_rows = dense_grid_integrate_piece(&grid);
        float fitness = compute_fitness_sai(sai, &grid, filled_rows);
        if (fitness > best_fitness) {
          best_fitness = fitness;
          best_x = x;
          best_rot = rot;
        }
      }
    }
    dg->piece_rot = original_rot;    
  }

  sai->best_x = best_x;
  sai->best_rot = best_rot;
}

void simple_ai_adjust_position(struct simple_ai *sai, struct dense_grid *dg)
{
  int x = dg->piece_x;
  int rot = dg->piece_rot;
  int moved = 0;

  if (x < sai->best_x) {
    send_keypress(SDL_SCANCODE_RIGHT);
    moved = 1;
  }
  else if (x > sai->best_x) {
    send_keypress(SDL_SCANCODE_LEFT);
    moved = 1;
  }

  if (rot != sai->best_rot) {
    send_keypress(SDL_SCANCODE_UP);
    moved = 1;
  }

  if (!moved)
    send_keypress(SDL_SCANCODE_DOWN);
}

void simple_ai_adjust_position_virtual(
  struct simple_ai *sai, struct dense_grid *dg
)
{
  while (dg->piece_rot != sai->best_rot) {
    if (!dense_grid_rotate_piece(dg)) {
      dense_grid_rotate_piece_backwards_no_check(dg);
      break;
    }
  }

  while (dg->piece_x < sai->best_x) {
    if (!dense_grid_move_piece(dg, 1, 0)) {
      dense_grid_move_piece_no_check(dg, -1, 0);
      break;
    }
  }

  while (dg->piece_x > sai->best_x) {
    if (!dense_grid_move_piece(dg, -1, 0)) {
      dense_grid_move_piece_no_check(dg, 1, 0);
      break;
    }
  }
}

int benchmark_ais(
  unsigned int seed, float a, float b, float c, float d, float e, float f,
  float g
)
{
  struct ai *ai;
  struct simple_ai *sai;

  ai = ai_new(a, b, c, d, e, f, g);
  if (!ai)
    return 0;
  
  sai = simple_ai_new(a, b, c, d, e, f, g);
  if (!sai) {
    ai_delete(ai);
    return 0;
  }

  struct dense_grid dg;  
  int ai_score = 0, sai_score = 0;

  // run ai
  srand(seed);
  dense_grid_init(&dg);
  dense_grid_add_piece(&dg, pieces[rand() % 7], 6, 1);
  while (1) {
    ai_run(ai, &dg);
    ai_adjust_position_virtual(ai, &dg);
    while (dense_grid_move_piece(&dg, 0, 1)){
    }
    dense_grid_move_piece_no_check(&dg, 0, -1);
    ai_score += dense_grid_integrate_piece_fast(&dg);
    if (!dense_grid_add_piece(&dg, pieces[rand() % 7], 6, 1)) {
      break;
    }
  }

  // run simple ai
  srand(seed);
  dense_grid_init(&dg);
  dense_grid_add_piece(&dg, pieces[rand() % 7], 6, 1);
  while (1) {
    simple_ai_run(sai, &dg);
    simple_ai_adjust_position_virtual(sai, &dg);
    while (dense_grid_move_piece(&dg, 0, 1)){
    }
    dense_grid_move_piece_no_check(&dg, 0, -1);
    sai_score += dense_grid_integrate_piece_fast(&dg);
    if (!dense_grid_add_piece(&dg, pieces[rand() % 7], 6, 1)) {
      break;
    }
  }

  puts("----------------------------------------");
  printf("AI scored       : %d\n", ai_score);
  printf("Simple AI scored: %d\n", sai_score);
  puts("----------------------------------------");

  ai_delete(ai);
  simple_ai_delete(sai);

  return 1;
}