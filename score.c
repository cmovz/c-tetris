#include "score.h"
#include <stdio.h>

static void draw_surface(struct score *score)
{
  if (score->surface)
    SDL_FreeSurface(score->surface);
  
  char text[24] = {'0', '\0'};
  uint64_t points = score->points;
  char *ptr = text;
  do {
    *ptr++ = '0' + points % 10;
    points /= 10;
  } while (points);
  *ptr = '\0';
  char *start_ptr = text;
  while (start_ptr < --ptr) {
    char temp = *start_ptr;
    *start_ptr = *ptr;
    *ptr = temp;
    ++start_ptr;
  }

  int w = 0, h = 0;
  TTF_SizeText(score->font, text, &w, &h);
  score->dest_rect.h = h;
  score->dest_rect.w = w;

  struct SDL_Color color = {255, 255, 255, 255};
  score->surface = TTF_RenderText_Blended(score->font, text, color);
}

int score_init(struct score *score)
{
  score->points = 0;
  score->font = TTF_OpenFont("./images/OpenSans-Regular.ttf", 32);
  if (!score->font) {
    fprintf(stderr, "Error opening font: %s\n", TTF_GetError());
    return 0;
  }
  
  score->surface = NULL;
  draw_surface(score);
  return 1;
}

void score_destroy(struct score *score)
{
  TTF_CloseFont(score->font);
  SDL_FreeSurface(score->surface);
}

void score_add(struct score *score, uint64_t points)
{
  score->points += points;
  draw_surface(score);
}

void score_draw(struct score *score, SDL_Surface *dest_surface, int x, int y)
{
  score->dest_rect.x = x - score->dest_rect.w;
  score->dest_rect.y = y;
  SDL_BlitSurface(score->surface, NULL, dest_surface, &score->dest_rect);
}
