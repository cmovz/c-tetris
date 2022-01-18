#include "ai.h"
#include "pieces.h"
#include <stdlib.h>
#include <SDL2/SDL.h>

struct minmax {
  int min;
  int max;
};

static struct minmax find_min_max_x(struct dense_grid *dg);

static int float_cmp(const void *aptr, const void *bptr)
{
  const float *a = aptr, *b = bptr;
  return *a < *b ? -1 : 1;
}

static inline float compute_fitness(
  struct ai *ai, struct dense_grid *dg, int filled_rows
)
{
  return filled_rows * ai->a
    - dg->bumpiness * ai->b
    - dg->aggregate_height * ai->c
    - dg->holes * ai->d
    - dg->wells_depth * ai->e;
}

static void compute_median_fitness2(
  struct ai *ai, struct possible_fit *pf, struct possible_fit1 *pf1
)
{
  float fitnesses[190];
  int pos = 0;
  for (int p = 0; p < PIECE_COUNT; ++p) {
    struct dense_grid outer_grid = pf1->dg;
    dense_grid_add_piece(&outer_grid, pieces[p], 6, 1);
    for (int rot = 0; rot < pieces[p]->rot_count; ++rot) {
      outer_grid.piece_rot = rot;
      struct minmax mm = find_min_max_x(&outer_grid);
      for (int x = mm.min; x < mm.max; ++x, ++pos) {
        pf1->pfs2[pos].dg = outer_grid;
        pf1->pfs2[pos].dg.piece_x = x;

        int game_over = 0;
        while (dense_grid_move_piece(&pf1->pfs2[pos].dg, 0, 1)) { 
        }
        if (!dense_grid_move_piece(&pf1->pfs2[pos].dg, 0, -1))
          game_over = 1;
        
        if (game_over) {
          pf1->fitness = -10000000.0f;
          return;
        }
        else {
          int r = dense_grid_integrate_piece(&pf1->pfs2[pos].dg);
          fitnesses[pos] = compute_fitness(ai, &pf1->pfs2[pos].dg, r);
          pf1->pfs2[pos].fitness = fitnesses[pos];
        }
      }
    }
  }

  qsort(fitnesses, pos, sizeof fitnesses[0], float_cmp);
  int median_idx0 = (pos - 1) / 2;
  int median_idx1 = pos / 2;
  pf1->fitness = (fitnesses[median_idx0] + fitnesses[median_idx1]) / 2.0f;
  pf1->pfs2_len = pos;
}

static void compute_median_fitness1(struct ai *ai, struct possible_fit *pf)
{
  float fitnesses[190];
  int pos = 0;
  for (int p = 0; p < PIECE_COUNT; ++p) {
    struct dense_grid outer_grid = pf->dg;
    dense_grid_add_piece(&outer_grid, pieces[p], 6, 1);
    for (int rot = 0; rot < pieces[p]->rot_count; ++rot) {
      outer_grid.piece_rot = rot;
      struct minmax mm = find_min_max_x(&outer_grid);
      for (int x = mm.min; x < mm.max; ++x, ++pos) {
        pf->pfs1[pos].dg = outer_grid;
        pf->pfs1[pos].dg.piece_x = x;

        int game_over = 0;
        while (dense_grid_move_piece(&pf->pfs1[pos].dg, 0, 1)) { 
        }
        if (!dense_grid_move_piece(&pf->pfs1[pos].dg, 0, -1))
          game_over = 1;
        
        if (game_over) {
          pf->fitness = -10000000.0f;
          return;
        }
        else {
          dense_grid_integrate_piece_fast(&pf->pfs1[pos].dg);
          compute_median_fitness2(ai, pf, pf->pfs1 + pos);
          fitnesses[pos] = pf->pfs1[pos].fitness;
        }
      }
    }
  }

  qsort(fitnesses, pos, sizeof fitnesses[0], float_cmp);
  int median_idx0 = (pos - 1) / 2;
  int median_idx1 = pos / 2;
  pf->fitness = (fitnesses[median_idx0] + fitnesses[median_idx1]) / 2.0f;
  pf->pfs1_len = pos;
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

struct ai *ai_new(float a, float b, float c, float d, float e)
{
  struct ai *ai = malloc(sizeof *ai);
  if (!ai)
    return NULL;

  ai_init(ai, a, b, c, d, e);
  return ai;
}

void ai_delete(struct ai *ai)
{
  free(ai);
}

void ai_init(struct ai *ai, float a, float b, float c, float d, float e)
{
  ai->a = a;
  ai->b = b;
  ai->c = c;
  ai->d = d;
  ai->e = e;
  ai->best_x = 0;
  ai->best_rot = 0;
  ai->pfs_idx = -1;
}

void ai_run(struct ai *ai, struct dense_grid *dg)
{
  int pos = 0;

  if (ai->pfs_idx != -1 && dense_grid_equal(dg, &ai->pfs[ai->pfs_idx].dg)) {
    struct possible_fit1 *pfs1 = ai->pfs[ai->pfs_idx].pfs1;
    struct possible_fit1 *pf1 = NULL;
     for (int i = 0; i < 190; ++i) {
      if (pfs1[i].dg.piece_matrices == dg->piece_matrices) {
        if (!pf1)
          pf1 = pfs1 + i;
        else if (pfs1[i].fitness > pf1->fitness)
          pf1 = pfs1 + i;
      }
    }

    struct possible_fit *pf = ai->pfs;
    ai->pfs_idx = 0;
    pf->dg = pf1->dg;
    pf->pfs1_len = pf1->pfs2_len;
    for (int i = 0; i < 190; ++i) {
      pf->pfs1[i].dg = pf1->pfs2[i].dg;
      pf->pfs1[i].fitness = pf1->pfs2[i].fitness;
    }

    for (int i = 0; i < pf->pfs1_len; ++i) {
      compute_median_fitness2(ai, pf, pf->pfs1 + i);
    }
  }

  else {
    for (int rot = 0; rot < dg->piece_rot_count; ++rot) {
      int original_rot = dg->piece_rot;
      dg->piece_rot = rot;
      struct minmax mm = find_min_max_x(dg);
      for (int x = mm.min; x < mm.max; ++x, ++pos) {
        ai->pfs[pos].dg = *dg;
        ai->pfs[pos].dg.piece_x = x;

        int game_over = 0;
        while (dense_grid_move_piece(&ai->pfs[pos].dg, 0, 1)){
        }
        if (!dense_grid_move_piece(&ai->pfs[pos].dg, 0, -1))
          game_over = 1;
        
        if (game_over)
          ai->pfs[pos].fitness = -10000000.0f;
        else {
          dense_grid_integrate_piece_fast(&ai->pfs[pos].dg);
          compute_median_fitness1(ai, ai->pfs + pos);
        }
      }
      dg->piece_rot = original_rot;    
    }
  }

  struct possible_fit *chosen = ai->pfs;
  for (int i = 0; i < pos; ++i) {
    if (ai->pfs[i].fitness > chosen->fitness) {
      chosen = ai->pfs + i;
      ai->pfs_idx = i;
    }
  }

  ai->best_x = chosen->dg.piece_x;
  ai->best_rot = chosen->dg.piece_rot;
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