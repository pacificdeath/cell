#include "main.h"

static IntX2 map_generation_dig_randomly(State *state, IntX2 start, IntX2 goal, int walkable_flags) {
    IntX2 options[3];
    if (start.y > goal.y) {
        options[0] = (IntX2) { start.x, start.y - 1 };
        options[1] = (IntX2) { start.x - 1, start.y };
        options[2] = (IntX2) { start.x + 1, start.y };
    } else if (start.x > goal.x) {
        options[0] = (IntX2) { start.x, start.y - 1 };
        options[1] = (IntX2) { start.x - 1, start.y };
        options[2] = (IntX2) { start.x, start.y + 1 };
    } else if (start.y < goal.y) {
        options[0] = (IntX2) { start.x - 1, start.y };
        options[1] = (IntX2) { start.x, start.y + 1 };
        options[2] = (IntX2) { start.x + 1, start.y };
    } else if (start.x < goal.x) {
        options[0] = (IntX2) { start.x, start.y - 1 };
        options[1] = (IntX2) { start.x, start.y + 1 };
        options[2] = (IntX2) { start.x + 1, start.y };
    } else {
        return start;
    }
    int option_idx = GetRandomValue(0, 2);
    while (!is_cell_valid(state, options[option_idx], walkable_flags)) {
        option_idx = (option_idx + 1) % 3;
    }
    return options[option_idx];
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

    const int pivot_amount = 5;
    IntX2 pivots[pivot_amount];
    pivots[0] = (IntX2) { GRID_WIDTH / 2, GRID_HEIGHT / 2 };
    pivots[1] = (IntX2) { 1, 1 };
    pivots[2] = (IntX2) { GRID_WIDTH - 2, 1 };
    pivots[3] = (IntX2) { 1, GRID_HEIGHT - 1 };
    pivots[4] = (IntX2) { GRID_WIDTH - 2, GRID_HEIGHT - 1 };

    for (int pivot_idx = 0; pivot_idx < pivot_amount - 1; pivot_idx++) {
        IntX2 pos = pivots[pivot_idx];
        for (int i = 0; i < 5000; i++) {
            if (GetRandomValue(0, 1) == 0) {
                pos.x += GetRandomValue(-1, 1);
                if (pos.x < 0) {
                    pos.x = 0;
                } else if (pos.x >= GRID_WIDTH) {
                    pos.x = GRID_WIDTH - 1;
                }
            } else {
                pos.y += GetRandomValue(-1, 1);
                if (pos.y < 0) {
                    pos.y = 0;
                } else if (pos.y >= GRID_HEIGHT) {
                    pos.y = GRID_HEIGHT - 1;
                }
            }
            state->grid[pos.x][pos.y] = CELL_FLAG_WALKABLE;
        }
        IntX2 goal = pivots[pivot_idx + 1];
        while (intX2_neq(pos, goal)) {
            pos = map_generation_dig_randomly(state, pos, goal, CELL_FLAG_ANY);
            state->grid[pos.x][pos.y] = CELL_FLAG_WALKABLE;
        }
    }

    int used_pivots_flags = 0;

    {
        int random_idx = GetRandomValue(0, pivot_amount - 1);
        int pivot_flag = 1 << random_idx;
        used_pivots_flags |= pivot_flag;
        state->player.previous_position = pivots[random_idx];
        state->player.position = pivots[random_idx];
    }

    for (int i = 0; i < CREATURE_CAPACITY; i++) {
        int random_idx;
        int pivot_flag;
        do {
            random_idx = (random_idx + 1) % pivot_amount;
            pivot_flag = 1 << random_idx;
        }
        while (has_flag(used_pivots_flags, pivot_flag));
        used_pivots_flags |= pivot_flag;
        state->creatures[i].previous_position = pivots[random_idx];
        state->creatures[i].position = pivots[random_idx];
    }
}
