#include "action_handler.h"

void action_handler_init(
  struct action_handler *action_handler, struct grid *grid,
  uint32_t actions_per_sec
)
{
  action_handler->grid = grid;
  game_clock_init(&action_handler->game_clock, actions_per_sec);
  action_handler->move_left = 0;
  action_handler->move_right = 0;
  action_handler->rotate = 0;
  action_handler->accelerate = 0;
  action_handler->moved_left = 1;
  action_handler->moved_right = 1;
  action_handler->rotated = 1;
  action_handler->accelerated = 1;
}

void action_handler_handle_event(
  struct action_handler *action_handler, SDL_Event *event
)
{
  if (event->type == SDL_KEYDOWN) {
    switch (event->key.keysym.scancode) {
    case SDL_SCANCODE_LEFT:
      action_handler->move_left = 1;
      action_handler->moved_left = 0;
      break;
    case SDL_SCANCODE_RIGHT:
      action_handler->move_right = 1;
      action_handler->moved_right = 0;
      break;
    case SDL_SCANCODE_DOWN:
      action_handler->accelerate = 1;
      action_handler->accelerated = 0;
      break;
    case SDL_SCANCODE_UP:
      action_handler->rotate = 1;
      action_handler->rotated = 0;
      break;
    }
  }
  else if (event->type == SDL_KEYUP) {
    switch (event->key.keysym.scancode) {
    case SDL_SCANCODE_LEFT:
      action_handler->move_left = 0;
      break;
    case SDL_SCANCODE_RIGHT:
      action_handler->move_right = 0;
      break;
    case SDL_SCANCODE_DOWN:
      action_handler->accelerate = 0;
      break;
    case SDL_SCANCODE_UP:
      action_handler->rotate = 0;
      break;
    }
  }
}

void action_handler_execute_actions(struct action_handler *action_handler)
{
  uint32_t ticks = game_clock_get_ticks(&action_handler->game_clock);
  for (uint32_t i = 0; i < ticks; ++i) {
    if (action_handler->move_left || !action_handler->moved_left) {
      action_handler->moved_left = 1;
      if (!grid_move_piece(action_handler->grid, -1, 0)) 
        grid_move_piece(action_handler->grid, 1, 0);
    }
    if (action_handler->move_right || !action_handler->moved_right) {
      action_handler->moved_right = 1;
      if (!grid_move_piece(action_handler->grid, 1, 0))
        grid_move_piece(action_handler->grid, -1, 0);
    }
    if (action_handler->accelerate || !action_handler->accelerated) {
      action_handler->accelerated = 1;
      if (!grid_move_piece(action_handler->grid, 0, 1))
        grid_move_piece(action_handler->grid, 0, -1);
    }
    if (action_handler->rotate || !action_handler->rotated) {
      action_handler->rotated = 1;
      if (!grid_rotate_piece(action_handler->grid))
        grid_rotate_piece_backwards(action_handler->grid);
    }
  }
}