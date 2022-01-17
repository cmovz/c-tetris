#include "textures.h"

SDL_Surface *textures[COLOR_COUNT];
static const char *paths[COLOR_COUNT] = {
  "./images/dark_cyan.bmp",
  "./images/dark_red.bmp",
  "./images/dark_brown.bmp",
  "./images/dark_magenta.bmp",
  "./images/dark_gray.bmp",
  "./images/dark_green.bmp",
  "./images/dark_blue.bmp",
  "./images/gray.bmp",
  "./images/black.bmp"
};

int textures_init(void)
{
  for (int i = 0; i < COLOR_COUNT; ++i) {
    textures[i] = SDL_LoadBMP(paths[i]);
    if (!textures[i]) {
      while (--i >= 0) {
        SDL_FreeSurface(textures[i]);
      }
      return 0;
    }
  }

  return 1;
}

void textures_quit(void)
{
  for (int i = 0; i < COLOR_COUNT; ++i) {
    SDL_FreeSurface(textures[i]);
  }
}