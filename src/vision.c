#include <math.h>
#include "main.h"

void raycast_discover_cells(State *state, IntX2 start, IntX2 end) {
    const float resolution = manhattan_distance(start, end) * 2;
    for (int fract_idx = 0; fract_idx <= resolution; fract_idx++) {
        float fract = (float)fract_idx / resolution;
        IntX2 ray_cell = {
            start.x + ((end.x - start.x) * fract),
            start.y + ((end.y - start.y) * fract)
        };
        if (is_cell_out_of_bounds(state, ray_cell)) {
            break;
        }
        uint8 *flags = &state->grid[ray_cell.x][ray_cell.y];
        *flags |= (CELL_FLAG_DISCOVERED | CELL_FLAG_VISIBLE);
        if (has_flag(*flags, CELL_FLAG_WALL)) {
            break;
        }
    }
}

void bresenham(State *state, IntX2 start, IntX2 end) {
    int dx =  abs(end.x - start.x), sx = start.x < end.x ? 1 : -1;
    int dy = -abs(end.y - start.y), sy = start.y < end.y ? 1 : -1;
    int err = dx + dy, e2;
    while (true) {
        IntX2 ray_cell = { start.x, start.y };
        if (is_cell_out_of_bounds(state, ray_cell)) {
            break;
        }
        uint8 *flags = &state->grid[ray_cell.x][ray_cell.y];
        *flags |= (CELL_FLAG_DISCOVERED | CELL_FLAG_VISIBLE);
        if (has_flag(*flags, CELL_FLAG_WALL)) {
            break;
        }
        if (start.x == end.x && start.y == end.y) break;
        e2 = 2*err;
        if (e2 >= dy) { err += dy; start.x += sx; }
        if (e2 <= dx) { err += dx; start.y += sy; }
    }
}

void discover_visible_cells(State *state) {
    IntX2 player = {
        state->player.position.x,
        state->player.position.y,
    };
    int game_right = state->game_offset.x + GAME_WIDTH;
    int game_bottom = state->game_offset.y + GAME_HEIGHT;
    for (int x = state->game_offset.x; x < game_right; x++) {
        for (int y = state->game_offset.y; y < game_bottom; y++) {
            IntX2 cell = (IntX2) { x, y };
            if (is_cell_out_of_bounds(state, cell)) {
                continue;
            }
            state->grid[x][y] &= ~CELL_FLAG_VISIBLE;
        }
    }

    int x_start = (state->game_offset.x > 0) ? state->game_offset.x : 0;
    int x_end = (game_right >= GRID_WIDTH) ? GRID_WIDTH : game_right;

    for (int x = x_start; x < x_end; x++) {
        bool is_left_or_right_edge = (x == x_start || x == x_end - 1);

        int y_start = (state->game_offset.y > 0) ? state->game_offset.y : 0;
        int y_end = (game_bottom >= GRID_HEIGHT) ? GRID_HEIGHT : game_bottom;

        if (is_left_or_right_edge) {

            for (int y = y_start; y < y_end; y++) {
                IntX2 cell = { x, y };
                bresenham(state, player, cell);
            }
            continue;
        }

        IntX2 first_cell_in_column = { x, y_start };
        bresenham(state, player, first_cell_in_column);

        IntX2 last_cell_in_column = { x, y_end - 1 };
        bresenham(state, player, last_cell_in_column);
    }
}
