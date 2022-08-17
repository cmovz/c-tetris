#include "core.h"
#include "textures.h"
#include "grid.h"
#include "pieces.h"
#include "game_clock.h"
#include "action_handler.h"
#include "score.h"
#include "ai.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#define FALLING_PER_SECOND 1
#define ACTIONS_PER_SECOND 12
#define MAX_FPS 60
#define DRAW_TIME (1000000000 / MAX_FPS)

#define A 0.103831f
#define B 0.164168f
#define C 0.012872f
#define D 0.962466f
#define E 0.206230f
#define F 0.000000f
#define G 0.004796f

static void update_time(void)
{
  struct timespec tp;
  clock_gettime(CLOCK_MONOTONIC, &tp);
  now = tp.tv_sec * 1000000000 + tp.tv_nsec;
}

static void run_game(int use_ai)
{
  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    fprintf(stderr, "%s\n", SDL_GetError());
    FATAL;
  }
  if (TTF_Init() == -1) {
    FATAL;
  }
  if (!textures_init()) {
    FATAL;
  }
  update_time();
  srand(time(NULL));

  SDL_Window *window = SDL_CreateWindow(
    "Tetris", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 12*24, 22*24, 0
  );
  SDL_Surface *window_surface = SDL_GetWindowSurface(window);

  struct grid grid;
  grid_init(&grid);
  grid_add_piece(&grid, pieces[rand() % 7], 6, 1);

  struct ai *ai = NULL;
  if (use_ai) {
    ai = ai_new(A, B, C, D, E, F, G);
    if (!ai)
      FATAL;
    
    if (!ai_init_worker())
      FATAL;

    if (!ai_run_async(ai, &grid.dense_grid))
      FATAL;
  }

  struct score score;
  if (!score_init(&score))
    FATAL;

  struct game_clock game_clock;
  game_clock_init(&game_clock, FALLING_PER_SECOND);

  struct action_handler action_handler;
  action_handler_init(&action_handler, &grid, ACTIONS_PER_SECOND);

  int running = 1;
  while (running) {
    update_time();
    uint64_t t0 = now;

    if (use_ai)
      ai_adjust_position(ai, &grid.dense_grid);

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) {
        running = 0;
        break;
      }
      action_handler_handle_event(&action_handler, &event);
    }

    action_handler_execute_actions(&action_handler);

    for (uint32_t i = 0; i < game_clock_get_ticks(&game_clock); ++i) {
      if (grid_move_piece(&grid, 0, 1))
        continue;
      
      grid_move_piece(&grid, 0, -1);
      score_add(&score, grid_integrate_piece(&grid));
      if (!grid_add_piece(&grid, pieces[rand() % 7], 6, 1)) {
        puts("----------------------------------------");
        puts("Game Over");
        printf("You scored: %lu\n", score.points);
        puts("----------------------------------------");
        running = 0;
        break;
      }
      if (use_ai && !ai_run_async(ai, &grid.dense_grid))
        FATAL;
    }

    grid_draw(&grid, window_surface);
    score_draw(&score, window_surface, 11 * 24 - 8, 24);
    SDL_UpdateWindowSurface(window);

    update_time();
    uint64_t dt = now - t0;
    if (dt < DRAW_TIME)
      SDL_Delay((DRAW_TIME - dt) / 1000000);
  }

  if (ai)
    ai_delete(ai);
  
  score_destroy(&score);
  textures_quit();
  SDL_DestroyWindow(window);
  TTF_Quit();
  SDL_Quit();
}

int main(int argc, char *argv[])
{
  if (argc > 2) {
    fprintf(stderr, "Usage: %s [--ai]\n", argv[0]);
    return -1;
  }
  int use_ai = 0;
  if (argc == 2) {
    if (strcmp("--ai", argv[1]) == 0)
      use_ai = 1;
    else if (strcmp("--benchmark-ais", argv[1]) == 0) {
      benchmark_ais(time(NULL), A, B, C, D, E, F, G);
      return 0;
    }
    else {
      fprintf(stderr, "Usage: %s [--ai]\n", argv[0]);
      return -1;
    }
  }

  run_game(use_ai);

  return 0;
}