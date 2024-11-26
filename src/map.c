#include "main.h"

static void dig_randomly(State *state, IntX2 start, IntX2 goal, int walkable_flags) {
    IntX2 cell = start;
    for (int i = 0; i < 5000; i++) {
        if (GetRandomValue(0, 1) == 0) {
            cell.x += GetRandomValue(-1, 1);
            if (cell.x < 0) {
                cell.x = 0;
            } else if (cell.x >= GRID_WIDTH) {
                cell.x = GRID_WIDTH - 1;
            }
        } else {
            cell.y += GetRandomValue(-1, 1);
            if (cell.y < 0) {
                cell.y = 0;
            } else if (cell.y >= GRID_HEIGHT) {
                cell.y = GRID_HEIGHT - 1;
            }
        }
        state->grid[cell.x][cell.y] = CELL_FLAG_WALKABLE;
    }
    while (intX2_neq(cell, goal)) {
        IntX2 options[3];
        if (cell.y > goal.y) {
            options[0] = (IntX2) { cell.x, cell.y - 1 };
            options[1] = (IntX2) { cell.x - 1, cell.y };
            options[2] = (IntX2) { cell.x + 1, cell.y };
        } else if (cell.x > goal.x) {
            options[0] = (IntX2) { cell.x, cell.y - 1 };
            options[1] = (IntX2) { cell.x - 1, cell.y };
            options[2] = (IntX2) { cell.x, cell.y + 1 };
        } else if (cell.y < goal.y) {
            options[0] = (IntX2) { cell.x - 1, cell.y };
            options[1] = (IntX2) { cell.x, cell.y + 1 };
            options[2] = (IntX2) { cell.x + 1, cell.y };
        } else if (cell.x < goal.x) {
            options[0] = (IntX2) { cell.x, cell.y - 1 };
            options[1] = (IntX2) { cell.x, cell.y + 1 };
            options[2] = (IntX2) { cell.x + 1, cell.y };
        }
        int option_idx = GetRandomValue(0, 2);
        while (!is_cell_valid(state, options[option_idx], CELL_FLAG_ANY)) {
            option_idx = (option_idx + 1) % 3;
        }
        cell = options[option_idx];
        for (int x = 0; x < 3; x++) {
            for (int y = 0; y < 3; y++) {
                IntX2 surrounding_cell = {
                    cell.x - 1 + x,
                    cell.y - 1 + y
                };
                if (is_cell_valid(state, surrounding_cell, CELL_FLAG_WALKABLE)) {
                    state->grid[surrounding_cell.x][surrounding_cell.y] = CELL_FLAG_WALKABLE;
                }
            }
        }
    }
}

void generate_map(State *state) {
    for (int x = 0; x < GRID_WIDTH; x++) {
        for (int y = 0; y < GRID_HEIGHT; y++) {
            state->grid[x][y] = CELL_FLAG_WALL;
        }
    }

    {
        IntX2 center = { (GRID_WIDTH / 2), (GRID_HEIGHT / 2) };
        for (int x = center.x - 2; x < center.x + 2; x++) {
            for (int y = center.y - 2; y < center.y + 2; y++) {
                state->grid[x][y] = CELL_FLAG_WALKABLE;
            }
        }
    }

    const int pivot_box_size = 3;
    for (int x = 0; x < pivot_box_size; x++) {
        for (int y = 0; y < pivot_box_size; y++) {
            int x2 = GRID_WIDTH - pivot_box_size + x;
            int y2 = GRID_HEIGHT - pivot_box_size + y;
            state->grid[x][y] = CELL_FLAG_WALKABLE;
            state->grid[x2][y] = CELL_FLAG_WALKABLE;
            state->grid[x][y2] = CELL_FLAG_WALKABLE;
            state->grid[x2][y2] = CELL_FLAG_WALKABLE;
        }
    }

    const int pivot_amount = 7;
    IntX2 pivots[pivot_amount];
    pivots[0] = (IntX2) { GRID_WIDTH / 2, GRID_HEIGHT / 2 };
    pivots[1] = (IntX2) { 1, 1 };
    pivots[2] = (IntX2) { GRID_WIDTH - 2, 1 };
    pivots[3] = (IntX2) { 1, GRID_HEIGHT - 1 };
    IntX2 random_range = {
        GRID_WIDTH / 4,
        GRID_HEIGHT / 4,
    };
    for (int i = 4; i < pivot_amount; i++) {
        pivots[i] = (IntX2) {
            GetRandomValue(random_range.x, (GRID_WIDTH - 1) - random_range.x),
            GetRandomValue(random_range.y, (GRID_HEIGHT - 1) - random_range.y)
        };
    }

    int used_pivots_flags = 0;

    {
        int random_idx = GetRandomValue(0, pivot_amount - 1);
        int pivot_flag = 1 << random_idx;
        used_pivots_flags |= pivot_flag;
        state->player.previous_position = pivots[random_idx];
        state->player.position = pivots[random_idx];
    }

    for (int pivot_idx = 0; pivot_idx < pivot_amount - 1; pivot_idx++) {
        int random_idx;
        int pivot_flag;

        do {
            random_idx = (random_idx + 1) % pivot_amount;
            pivot_flag = 1 << random_idx;
        } while (has_flag(used_pivots_flags, pivot_flag));

        IntX2 pos = pivots[pivot_idx];
        IntX2 goal = pivots[pivot_idx + 1];
        dig_randomly(state, pos, goal, CELL_FLAG_ANY);

        if (pivot_idx < CREATURE_CAPACITY) {
            state->creatures[pivot_idx].previous_position = pivots[random_idx];
            state->creatures[pivot_idx].position = pivots[random_idx];
        }

        used_pivots_flags |= pivot_flag;
    }
}
