#ifndef SCORE_H
#define SCORE_H

#include <stdint.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

struct score {
  uint64_t points;
  TTF_Font *font;
  SDL_Surface *surface;
  SDL_Rect dest_rect;
};

int score_init(struct score *score);
void score_destroy(struct score *score);

void score_add(struct score *score, uint64_t points);
void score_draw(struct score *score, SDL_Surface *dest_surface, int x, int y);

#endif