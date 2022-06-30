#include "grid.h"
#include "colors.h"
#include "textures.h"
#include <stdlib.h>
#include <string.h>

static struct dense_grid default_dense_grid = {
  {
    0xe007e007e007ffff,0xe007e007e007e007,0xe007e007e007e007,
    0xe007e007e007e007,0xe007e007e007e007,0xffffffffffffe007
  },
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static int dense_grid_check_collision(struct dense_grid *dg)
{
  uint16_t pc_matrix = (dg->piece_matrices >> (dg->piece_rot << 4)) & 0xffff;
  int pc_y = 0;
  for (int y = dg->piece_y; y < dg->piece_y + 4; ++y, ++pc_y) {
    uint64_t pc_matrix_line = ((pc_matrix >> (pc_y << 2)) & 0xf) << dg->piece_x;
    uint64_t mask = pc_matrix_line << (y << 4);
    if (dg->cells[y >> 2] & mask)
      return 1;
  }
  return 0;
}

static void dense_grid_compute_stats(struct dense_grid *dg)
{
  uint8_t heights[16] = {20, 20, 20, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 20, 20, 20};
  int hole_count = 0;
  int aggregate_height = 0;
  dg->blocked_wells_depth = 0;
  dg->blocking_wells = 0;
  for (int x = 3; x < 13; ++x) {
    int blocks_above = 0;
    int well_depth = 0;
    int is_filled_above = 0;
    for (int y = 1; y < 21; ++y) {
      int idx = y >> 2;
      int pos = (y << 4) + x;
      int shift = pos & 0x3f;
      int filled = (dg->cells[idx] >> shift) & 1;
      if (filled) {
        if (is_filled_above)
          ++blocks_above;
        else {
          if (well_depth > 1) {
            dg->blocked_wells_depth += well_depth;
            dg->blocking_wells += blocks_above;
          }
          blocks_above = 1;
          well_depth = 0;
        }

        if (heights[x] == 0) {
          int h = 21 - y;
          heights[x] = h;
          aggregate_height += h;
        }

        is_filled_above = 1;
      }
      else {
        if (heights[x] != 0) {
          ++hole_count;

          if (is_filled_above)
            well_depth = 1;
          else if (well_depth > 0)
            ++well_depth;
        }
        is_filled_above = 0;
      }
    }
    if (well_depth > 1) {
      dg->blocked_wells_depth += well_depth;
      dg->blocking_wells += blocks_above;
    }
  }

  // compute bumpiness
  int bumpiness = 0;
  for (int x = 4; x < 13; ++x)
    bumpiness += abs(heights[x] - heights[x-1]);
  
  // compute wells depth
  int wells_depth = 0;
  for (int x = 3; x < 13; ++x) {
    int minh = heights[x-1] < heights[x+1] ? heights[x-1] : heights[x+1];
    int dh = minh - heights[x];
    if (dh >= 3)
      wells_depth += dh;
  }

  dg->holes = hole_count;
  dg->aggregate_height = aggregate_height;
  dg->bumpiness = bumpiness;
  dg->wells_depth = wells_depth;
}

int dense_grid_integrate_piece_fast(struct dense_grid *dg)
{
  uint16_t pc_matrix = (dg->piece_matrices >> (dg->piece_rot << 4)) & 0xffff;
  int pc_matrix_pos = 0;
  for (int my = 0; my < 4; ++my) {
    int y = my + dg->piece_y;
    int dg_idx = y >> 2;
    int y_shift = (y & 3) << 4;
    for (int mx = 0; mx < 4; ++mx, ++pc_matrix_pos) {
      uint64_t bit = (pc_matrix >> pc_matrix_pos) & 1;      
      dg->cells[dg_idx] |= bit << (y_shift + dg->piece_x + mx);
    }
  }

  // clear filled lines
  int filled_lines = 0;
  int total_filled_lines = 0;
  int y = dg->piece_y + 3;
  for (int my = 3; my >= 0; --my, --y) {
    if (y > 20)
      continue;

    int dg_idx = y >> 2;
    uint64_t filled_line_mask = (uint64_t)0xffff << ((y & 3) << 4);
    if ((dg->cells[dg_idx] & filled_line_mask) == filled_line_mask) {
      ++filled_lines;
      ++total_filled_lines;
    }
    else if (filled_lines) {
      void *dest = (char*)dg->cells + 2 + (filled_lines << 1);
      memmove(dest, (char*)dg->cells + 2, y << 1);
      y += filled_lines;
      filled_lines = 0;
    }
  }
  if (filled_lines) {
    void *dest = (char *)dg->cells + 2 + (filled_lines << 1);
    memmove(dest, (char *)dg->cells + 2, y << 1);
    filled_lines = 0;
  }
  for (int y = 1; y < total_filled_lines + 1; ++y) {
    int dg_idx = y >> 2;
    uint64_t clear_mask = ~((uint64_t)0xffff << ((y & 3) << 4));
    uint64_t line_mask = (uint64_t)0xe007 << ((y & 3) << 4);
    dg->cells[dg_idx] &= clear_mask;
    dg->cells[dg_idx] |= line_mask;
  }

  return total_filled_lines;
}

int dense_grid_integrate_piece(struct dense_grid * dg)
{
  uint16_t pc_matrix = (dg->piece_matrices >> (dg->piece_rot << 4)) & 0xffff;
  int pc_matrix_pos = 0;
  for (int my = 0; my < 4; ++my) {
    int y = my + dg->piece_y;
    int dg_idx = y >> 2;
    int y_shift = (y & 3) << 4;
    for (int mx = 0; mx < 4; ++mx, ++pc_matrix_pos) {
      uint64_t bit = (pc_matrix >> pc_matrix_pos) & 1;      
      dg->cells[dg_idx] |= bit << (y_shift + dg->piece_x + mx);
    }
  }

  // clear filled lines
  int filled_lines = 0;
  int total_filled_lines = 0;
  int y = dg->piece_y + 3;
  for (int my = 3; my >= 0; --my, --y) {
    if (y > 20)
      continue;

    int dg_idx = y >> 2;
    uint64_t filled_line_mask = (uint64_t)0xffff << ((y & 3) << 4);
    if ((dg->cells[dg_idx] & filled_line_mask) == filled_line_mask) {
      ++filled_lines;
      ++total_filled_lines;
    }
    else if (filled_lines) {
      void *dest = (char*)dg->cells + 2 + (filled_lines << 1);
      memmove(dest, (char*)dg->cells + 2, y << 1);
      y += filled_lines;
      filled_lines = 0;
    }
  }
  if (filled_lines) {
    void *dest = (char *)dg->cells + 2 + (filled_lines << 1);
    memmove(dest, (char *)dg->cells + 2, y << 1);
    filled_lines = 0;
  }
  for (int y = 1; y < total_filled_lines + 1; ++y) {
    int dg_idx = y >> 2;
    uint64_t clear_mask = ~((uint64_t)0xffff << ((y & 3) << 4));
    uint64_t line_mask = (uint64_t)0xe007 << ((y & 3) << 4);
    dg->cells[dg_idx] &= clear_mask;
    dg->cells[dg_idx] |= line_mask;
  }

  dense_grid_compute_stats(dg);

  return total_filled_lines;
}

int dense_grid_equal(const struct dense_grid *a, const struct dense_grid *b)
{
  return memcmp(a, b, 48) == 0;
}

void dense_grid_init(struct dense_grid *dg)
{
  *dg = default_dense_grid;
}

void grid_init(struct grid *grid)
{
  grid->dense_grid = default_dense_grid;
  for (int pos = 0; pos < 64 * 6; ++pos) {
    int is_filled = (grid->dense_grid.cells[pos >> 6] >> (pos & 0x3f)) & 1;
    int color = is_filled ? GRAY : BLACK;
    int cc_index = pos >> 1;
    int cc_shift = (pos & 1) == 0 ? 0 : 4;
    if (cc_shift == 0)
      grid->colored_cells[cc_index] = 0x00;
    
    grid->colored_cells[cc_index] |= color << cc_shift;
  }
  grid->piece.matrices = 0;
}

int dense_grid_add_piece(
  struct dense_grid *dg, struct piece *piece, uint8_t x, uint8_t y
)
{
  dg->piece_matrices = piece->matrices;
  dg->piece_x = x;
  dg->piece_y = y;

  int rot;
  for (rot = 0; rot < 4; ++rot) {
    dg->piece_rot = rot;
    if (!dense_grid_check_collision(dg))
      break;
  }
  if (rot == 4)
    return 0;

  dg->piece_rot_count = piece->rot_count;
  return 1;
}

int grid_add_piece(struct grid *grid, struct piece *piece, uint8_t x, uint8_t y)
{
  grid->dense_grid.piece_matrices = piece->matrices;
  grid->dense_grid.piece_x = x;
  grid->dense_grid.piece_y = y;

  int rot;
  for (rot = 0; rot < 4; ++rot) {
    grid->dense_grid.piece_rot = rot;
    if (!dense_grid_check_collision(&grid->dense_grid))
      break;
  }
  if (rot == 4)
    return 0;

  grid->dense_grid.piece_rot_count = piece->rot_count;
  grid->piece = *piece;
  return 1;
}

int dense_grid_rotate_piece(struct dense_grid *dg)
{
  dg->piece_rot = (dg->piece_rot + 1) & 3;
  return !dense_grid_check_collision(dg);
}

void dense_grid_rotate_piece_backwards_no_check(struct dense_grid *dg)
{
  dg->piece_rot = (dg->piece_rot + 3) & 3;
}

int grid_rotate_piece(struct grid *grid)
{
  grid->dense_grid.piece_rot = (grid->dense_grid.piece_rot + 1) & 3;
  return !dense_grid_check_collision(&grid->dense_grid);
}

int grid_rotate_piece_backwards(struct grid *grid)
{
  grid->dense_grid.piece_rot = (grid->dense_grid.piece_rot + 3) & 3;
  return !dense_grid_check_collision(&grid->dense_grid);
}

int dense_grid_move_piece(struct dense_grid *dg, int x, int y)
{
  dg->piece_x += x;
  dg->piece_y += y;
  return !dense_grid_check_collision(dg);
}

void dense_grid_move_piece_no_check(struct dense_grid *dg, int x, int y)
{
  dg->piece_x += x;
  dg->piece_y += y;
}

int grid_move_piece(struct grid *grid, int x, int y)
{
  grid->dense_grid.piece_x += x;
  grid->dense_grid.piece_y += y;
  return !dense_grid_check_collision(&grid->dense_grid);
}

int grid_integrate_piece(struct grid *grid)
{
  struct dense_grid *const dg = &grid->dense_grid;
  uint16_t pc_matrix = (dg->piece_matrices >> (dg->piece_rot << 4)) & 0xffff;
  int pc_matrix_pos = 0;
  for (int my = 0; my < 4; ++my) {
    int y = my + dg->piece_y;
    int dg_idx = y >> 2;
    int y_shift = (y & 3) << 4;
    for (int mx = 0; mx < 4; ++mx, ++pc_matrix_pos) {
      uint64_t bit = (pc_matrix >> pc_matrix_pos) & 1;      
      dg->cells[dg_idx] |= bit << (y_shift + dg->piece_x + mx);

      if (bit) {
        int pos = (y << 4) + dg->piece_x + mx;
        int cc_idx = pos >> 1;
        int cc_shift = (pos & 1) == 0 ? 0 : 4;
        grid->colored_cells[cc_idx] &= ~(0xf << cc_shift);
        grid->colored_cells[cc_idx] |= grid->piece.color << cc_shift;
      }
    }
  }

  // clear filled lines
  int filled_lines = 0;
  int total_filled_lines = 0;
  int y = dg->piece_y + 3;
  for (int my = 3; my >= 0; --my, --y) {
    if (y > 20)
      continue;

    int dg_idx = y >> 2;
    uint64_t filled_line_mask = (uint64_t)0xffff << ((y & 3) << 4);
    if ((dg->cells[dg_idx] & filled_line_mask) == filled_line_mask) {
      ++filled_lines;
      ++total_filled_lines;
    }
    else if (filled_lines) {
      void *dest = (char*)dg->cells + 2 + (filled_lines << 1);
      memmove(dest, (char*)dg->cells + 2, y << 1);
      void *cc_dest = grid->colored_cells + 8 + (filled_lines << 3);
      memmove(cc_dest, grid->colored_cells + 8, y << 3);
      y += filled_lines;
      filled_lines = 0;
    }
  }
  if (filled_lines) {
    void *dest = (char *)dg->cells + 2 + (filled_lines << 1);
    memmove(dest, (char *)dg->cells + 2, y << 1);
    void *cc_dest = grid->colored_cells + 8 + (filled_lines << 3);
    memmove(cc_dest, grid->colored_cells + 8, y << 3);
    filled_lines = 0;
  }
  for (int y = 1; y < total_filled_lines + 1; ++y) {
    int dg_idx = y >> 2;
    uint64_t clear_mask = ~((uint64_t)0xffff << ((y & 3) << 4));
    uint64_t line_mask = (uint64_t)0xe007 << ((y & 3) << 4);
    dg->cells[dg_idx] &= clear_mask;
    dg->cells[dg_idx] |= line_mask;

    for (int x = 0; x < 16; ++x) {
      int color = BLACK;
      if (x < 3 || x > 12) {
        color = GRAY;
      }
      int pos = (y << 4) + x;
      int shift = (pos & 1) == 0 ? 0 : 4;
      int idx = pos >> 1;
      if (shift == 0)
        grid->colored_cells[idx] = 0x00;
      
      grid->colored_cells[idx] |= color << shift;
    }
  }

  dense_grid_compute_stats(dg);

  return total_filled_lines;
}

void grid_draw(struct grid *grid, SDL_Surface *surface)
{
  SDL_Rect dest_rect = {0, 0, 24, 24};
  for (int y = 0; y < 22; ++y) {
    for (int x = 2; x < 14; ++x) {
      int pos = (y << 4) + x;
      int cc_index = pos >> 1;
      int cc_shift = (pos & 1) == 0 ? 0 : 4;
      int color = (grid->colored_cells[cc_index] >> cc_shift) & 0xf;
      SDL_BlitSurface(textures[color], NULL, surface, &dest_rect);
      dest_rect.x += 24;
    }
    dest_rect.x = 0;
    dest_rect.y += 24;
  }

  struct dense_grid *const dg = &grid->dense_grid;
  int pc_color = grid->piece.color;
  uint16_t pc_matrix = (dg->piece_matrices >> (dg->piece_rot << 4)) & 0xffff;
  int pc_matrix_pos = 0;
  for (int pc_y = 0; pc_y < 4; ++pc_y) {
    for (int pc_x = 0; pc_x < 4; ++pc_x, ++pc_matrix_pos) {
      if (((pc_matrix >> pc_matrix_pos) & 1) == 0)
        continue;
      
      dest_rect.x = (dg->piece_x + pc_x - 2) * 24;
      dest_rect.y = (dg->piece_y + pc_y) * 24;
      SDL_BlitSurface(textures[pc_color], NULL, surface, &dest_rect);
    }
  }
}

void grid_print(struct grid *grid)
{
  for (int pos = 0; pos < 64 * 6; ++pos) {
    int is_filled = (grid->dense_grid.cells[pos >> 6] >> (pos & 0x3f)) & 1;
    if (is_filled) {
      putchar('1');
    }
    else {
      putchar(' ');
    }
    if ((pos & 0xf) == 0xf)
      putchar('\n');
  }
}