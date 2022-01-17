#ifndef ACTION_HANDLER_H
#define ACTION_HANDLER_H

#include "game_clock.h"
#include "grid.h"
#include <SDL2/SDL.h>

struct action_handler {
  struct game_clock game_clock;
  struct grid *grid;
  uint8_t move_left;
  uint8_t move_right;
  uint8_t rotate;
  uint8_t accelerate;
  uint8_t moved_left;
  uint8_t moved_right;
  uint8_t rotated;
  uint8_t accelerated;
};

void action_handler_init(
  struct action_handler *action_handler, struct grid *grid,
  uint32_t actions_per_sec
);

void action_handler_handle_event(
  struct action_handler *action_handler, SDL_Event *event
);

void action_handler_execute_actions(struct action_handler *action_handler);

#endif