#ifndef GAME_CLOCK_H
#define GAME_CLOCK_H

#include "core.h"
#include <stdint.h>

struct game_clock {
  uint64_t prev_tick;
  uint64_t tick_time;
};

void game_clock_init(struct game_clock *game_clock, uint32_t ticks_per_sec);
uint32_t game_clock_get_ticks(struct game_clock *game_clock);

#endif