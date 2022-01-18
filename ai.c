#include "ai.h"
#include "pieces.h"
#include <SDL2/SDL.h>

struct minmax {
  int min;
  int max;
};

struct possible_fit {
  struct dense_grid dg;
  float fitness;
  int x;
  int rot;
};

static struct minmax find_min_max_x(struct dense_grid *dg);

static int pfs_comp_r(const void *aptr, const void *bptr)
{
  const struct possible_fit *a = aptr, *b = bptr;
  return a->fitness < b->fitness ? 1 : -1;
}

static int pfs_comp(const void *aptr, const void *bptr)
{
  const struct possible_fit *a = aptr, *b = bptr;
  return a->fitness < b->fitness ? -1 : 1;
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

static float compute_median_fitness(struct ai *ai, struct dense_grid *dg, int r)
{
  struct possible_fit pfs[40 * 7];
  int pos = 0;

  for (int p = 0; p < PIECE_COUNT; ++p) {
    struct dense_grid outer_grid = *dg;
    dense_grid_add_piece(&outer_grid, pieces[p], 6, 1);
    for (int rot = 0; rot < pieces[p]->rot_count; ++rot) {
      outer_grid.piece_rot = rot;
      struct minmax mm = find_min_max_x(&outer_grid);
      for (int x = mm.min; x < mm.max; ++x, ++pos) {
        pfs[pos].dg = outer_grid;
        pfs[pos].dg.piece_x = x;
        pfs[pos].x = x;
        pfs[pos].rot = rot;

        int game_over = 0;
        while (dense_grid_move_piece(&pfs[pos].dg, 0, 1)) { 
        }
        if (!dense_grid_move_piece(&pfs[pos].dg, 0, -1))
          game_over = 1;
        
        if (game_over)
          pfs[pos].fitness = -10000000.0f;
        else {
          if (r > 1) {
            int filled_rows = dense_grid_integrate_piece(&pfs[pos].dg);
            pfs[pos].fitness = compute_fitness(ai, &pfs[pos].dg, filled_rows);
          }
          else
            pfs[pos].fitness = compute_median_fitness(ai, &pfs[pos].dg, r + 1);
        }
      }
    }
  }

  qsort(pfs, pos, sizeof pfs[0], pfs_comp);
  int median_idx0 = (pos - 1) / 2;
  int median_idx1 = pos / 2;
  return (pfs[median_idx0].fitness + pfs[median_idx1].fitness) / 2.0f;
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

void ai_init(struct ai *ai, float a, float b, float c, float d, float e)
{
  ai->a = a;
  ai->b = b;
  ai->c = c;
  ai->d = d;
  ai->e = e;
  ai->best_x = 0;
  ai->best_rot = 0;
}

void ai_run(struct ai *ai, struct dense_grid *dg)
{
  struct possible_fit pfs[40];
  int pos = 0;

  for (int rot = 0; rot < dg->piece_rot_count; ++rot) {
    int original_rot = dg->piece_rot;
    dg->piece_rot = rot;
    struct minmax mm = find_min_max_x(dg);
    for (int x = mm.min; x < mm.max; ++x, ++pos) {
      pfs[pos].dg = *dg;
      pfs[pos].dg.piece_x = x;
      pfs[pos].x = x;
      pfs[pos].rot = rot;

      int game_over = 0;
      while (dense_grid_move_piece(&pfs[pos].dg, 0, 1)){
      }
      if (!dense_grid_move_piece(&pfs[pos].dg, 0, -1))
        game_over = 1;
      
      if (game_over)
        pfs[pos].fitness = -10000000.0f;
      else {
        int filled_rows = dense_grid_integrate_piece(&pfs[pos].dg);
        pfs[pos].fitness = compute_fitness(ai, &pfs[pos].dg, filled_rows);
      }
    }
    dg->piece_rot = original_rot;    
  }

  qsort(pfs, pos, sizeof pfs[0], pfs_comp_r);

  struct possible_fit *chosen = pfs;
  struct possible_fit *original_chosen = chosen;
  int max = pos < 20 ? pos : 20;
  for (int i = 0; i < max; ++i) {
    pfs[i].fitness = compute_median_fitness(ai, &pfs[i].dg, 1);
    if (pfs[i].fitness > chosen->fitness)
      chosen = pfs + i;
  }

  if (chosen != original_chosen)
    puts("different choice");
  else
    puts("same choice");

  ai->best_x = chosen->x;
  ai->best_rot = chosen->rot;
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