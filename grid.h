#ifndef GRID_H
#define GRID_H

#include "pieces.h"
#include <stdint.h>
#include <SDL2/SDL.h>

struct dense_grid {
  uint64_t cells[6];
  uint64_t piece_matrices;
  uint8_t piece_x;
  uint8_t piece_y;
  uint8_t piece_rot;
  uint8_t piece_rot_count;
  uint8_t aggregate_height;
  uint8_t bumpiness;
  uint8_t holes;
  uint8_t wells_depth;
};

struct grid {
  struct dense_grid dense_grid;
  uint8_t colored_cells[64 * 6 / 2];
  struct piece piece;
};

void grid_init(struct grid *grid);
int grid_add_piece(
  struct grid *grid, struct piece *piece, uint8_t x, uint8_t y
);
int grid_rotate_piece(struct grid *grid);
int grid_rotate_piece_backwards(struct grid *grid);
int grid_move_piece(struct grid *grid, int x, int y);
int grid_integrate_piece(struct grid *grid);
void grid_draw(struct grid *grid, SDL_Surface *surface);
void grid_print(struct grid *grid);

void dense_grid_init(struct dense_grid *dg);
int dense_grid_add_piece(
  struct dense_grid *dg, struct piece *piece, uint8_t x, uint8_t y
);
int dense_grid_rotate_piece(struct dense_grid *dg);
void dense_grid_rotate_piece_backwards_no_check(struct dense_grid *dg);
int dense_grid_move_piece(struct dense_grid *dg, int x, int y);
void dense_grid_move_piece_no_check(struct dense_grid *dg, int x, int y);
int dense_grid_integrate_piece_fast(struct dense_grid *dg);
int dense_grid_integrate_piece(struct dense_grid * dg);
int dense_grid_equal(const struct dense_grid *a, const struct dense_grid *b);

#endif