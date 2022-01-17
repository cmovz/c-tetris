#ifndef TEXTURES_H
#define TEXTURES_H

#include "colors.h"
#include <SDL2/SDL.h>

extern SDL_Surface *textures[COLOR_COUNT];

int textures_init(void);
void textures_quit(void);

#endif