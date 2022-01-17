#include "game_clock.h"

void game_clock_init(struct game_clock *game_clock, uint32_t ticks_per_sec)
{
  game_clock->prev_tick = now;
  game_clock->tick_time = 1000000000 / ticks_per_sec;
}

uint32_t game_clock_get_ticks(struct game_clock *game_clock)
{
  uint64_t dt = now - game_clock->prev_tick;
  if (dt < game_clock->tick_time)
    return 0;
  
  uint32_t ticks = dt / game_clock->tick_time;
  game_clock->prev_tick = now - (dt % game_clock->tick_time);
  return ticks;
}