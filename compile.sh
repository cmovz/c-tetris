gcc -o tetris -std=c99 -flto -O3 -Wall -Wno-switch *.c \
  `sdl2-config --cflags` `sdl2-config --libs` -lSDL2_ttf