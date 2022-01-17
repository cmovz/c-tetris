#include "pieces.h"

static struct piece i = {
  0xf00444400f02222, 2, DARK_CYAN
};

static struct piece j = {
  0x470022600710322, 4, DARK_RED
};

static struct piece l = {
  0x74022301700622, 4, DARK_BROWN
};

static struct piece o = {
  0x66006600660066, 1, DARK_MAGENTA
};

static struct piece s = {
  0x231036004620036, 2, DARK_GRAY
};

static struct piece t = {
  0x232027002620072, 4, DARK_GREEN
};

static struct piece z = {
  0x132063002640063, 2, DARK_BLUE
};

struct piece *pieces[PIECE_COUNT] = {&i, &j, &l, &o, &s, &t, &z};