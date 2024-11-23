#include <math.h>
#include "main.h"

void discover_visible_cells(State *state) {
    Cell player = {
        state->player.position.x,
        state->player.position.y,
    };
    for (int x = 0; x < GRID_WIDTH; x++) {
        for (int y = 0; y < GRID_HEIGHT; y++) {
            state->grid[x][y] &= ~CELL_BIT_VISIBLE;
        }
    }
    for (int x = 0; x < GRID_WIDTH; x++) {
        bool is_left_or_right_edge = x == 0 || x == GRID_WIDTH - 1;
        for (int y = 0; y < GRID_HEIGHT; y++) {
            bool is_top_edge = y == 0;
            if (!is_left_or_right_edge && !is_top_edge) {
                int bottom_edge = GRID_HEIGHT - 1;
                y = bottom_edge;
            }
            const float resolution = manhattan_distance(player, (Cell) { x, y }) * 2;
            for (int fract_idx = 0; fract_idx <= resolution; fract_idx++) {
                float fract = (float)fract_idx / resolution;
                Cell c = {
                    player.x + (x - player.x) * fract,
                    player.y + (y - player.y) * fract
                };
                if (c.x < 0 || c.x >= GRID_WIDTH ||
                    c.y < 0 || c.y >= GRID_HEIGHT
                ) {
                    break;
                }
                uint8 *flags = &state->grid[c.x][c.y];
                *flags |= (CELL_BIT_DISCOVERED | CELL_BIT_VISIBLE);
                if (has_bit(*flags, CELL_BIT_WALL)) {
                    break;
                }
            }
        }
    }
}
