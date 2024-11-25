#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "../raylib/include/raylib.h"

#include "main.h"
#include "map.c"
#include "vision.c"
#include "movement.c"
#include "renderer.c"

uint8 get_direction(IntX2 from, IntX2 to) {
    if (from.y > to.y) {
        return ORTHAGONAL_N;
    } else if (from.x > to.x) {
        return ORTHAGONAL_W;
    } else if (from.y < to.y) {
        return ORTHAGONAL_S;
    } else if (from.x < to.x) {
        return ORTHAGONAL_E;
    }
    return NO_DIRECTION;
}

void fill_cell(State *state, IntX2 position) {
    state->grid[position.x][position.y] &= ~CELL_FLAG_WALKABLE;
    state->grid[position.x][position.y] |= CELL_FLAG_WALKABLE;
}

void print_grid(State *state) {
    printf("   ");
    for (int x = 0; x < GRID_WIDTH; x++) {
        printf("%02i ", x);
    }
    printf("\n");
    for (int y = 0; y < GRID_HEIGHT; y++) {
        printf("%02i: ", y);
        for (int x = 0; x < GRID_WIDTH; x++) {
            printf("%-2x ", state->grid[x][y]);
        }
        printf("\n");
    }
}

void print_a_star(AStar *a, IntX2 start, IntX2 goal, int (* operation)(ANode *)) {
    printf("* * * * * * * * * * * *\n");
    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            ANode *n = &a->all_list[x][y];
            int something = operation(n);
            if (intX2_eq(n->position, start)) {
                printf("<@> ");
            } else if (intX2_eq(n->position, goal)) {
                printf("<X> ");
            } else if (something > 999) {
                printf("--- ");
            } else {
                printf("%03i ", something);
            }
        }
        printf("\n");
    }
}

void update_game_offset(State *state) {
    IntX2 local_dimensions = { GAME_WIDTH, GAME_HEIGHT };
    state->game_offset = intX2_subtract(state->player.position, intX2_divide(local_dimensions, 2));
}

int main(void) {
    const int screen_width = CELLSIZE * GAME_WIDTH;
    const int screen_height = CELLSIZE * GAME_HEIGHT;

    InitWindow(screen_width, screen_height, "Cell");

    SetTargetFPS(60);

    State *state = (State *)calloc(1, sizeof(State));

    state->player = (Creature) {
        .type = CREATURE_PLAYER,
    };

    state->creatures[0] = (Creature) {
        .type = CREATURE_EVIL_TRIANGLE,
        .direction = DIAGONAL_NE,
    };
    state->creatures[1] = (Creature) {
        .type = CREATURE_BIG_EVIL_TRIANGLE,
        .last_known_player_location = INVALID_CELL,
    };

    generate_map(state);

    update_game_offset(state);
    discover_visible_cells(state);

    int input_key = 0;

    while (!WindowShouldClose()) {
        update_game_offset(state);

        state->mouse_current = screen_to_game_position(state, GetMousePosition());

        bool left_mouse_pressed = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
        bool mouse_condition = left_mouse_pressed && intX2_neq(
            astar_path(state, state->player.position, state->mouse_current, CELL_FLAG_PLAYER_WALKABLE),
            state->player.position
        );

        if (IsKeyDown(KEY_UP)) {
            input_key = KEY_UP;
        } else if (IsKeyDown(KEY_LEFT)) {
            input_key = KEY_LEFT;
        } else if (IsKeyDown(KEY_DOWN)) {
            input_key = KEY_DOWN;
        } else if (IsKeyDown(KEY_RIGHT)) {
            input_key = KEY_RIGHT;
        }

        if (mouse_condition) {
            state->mouse_target = state->mouse_current;
        }

        bool wait = IsKeyPressed(KEY_SPACE);
        if (mouse_condition || input_key || wait) {
            state->flags |= GAME_FLAG_IS_MOVING;
        }

        float frame_time = GetFrameTime();
        state->animation_timer += frame_time;
        if (state->animation_timer >= TIME_PER_ANIMATION) {
            state->animation_timer -= TIME_PER_ANIMATION;
        }

        if (state->game_timer < TIME_PER_TURN) {
            state->game_timer += frame_time;
        } else if (state->game_timer > TIME_PER_TURN && !has_flag(state->flags, GAME_FLAG_READY_FOR_UPDATE)) {
            state->game_timer = TIME_PER_TURN;
            state->flags |= GAME_FLAG_READY_FOR_UPDATE;
            for (int i = 0; i < CREATURE_CAPACITY; i++) {
                Creature *c = &state->creatures[i];
                bool creature_visible = has_flag(c->flags, CREATURE_FLAG_VISIBLE);
                bool cell_visible = has_flag(state->grid[c->position.x][c->position.y], CELL_FLAG_VISIBLE);
                if (creature_visible && !cell_visible) {
                    c->flags &= ~CREATURE_FLAG_VISIBLE;
                }
            }
        }

        if (has_flag(state->flags, GAME_FLAG_READY_FOR_UPDATE | GAME_FLAG_IS_MOVING)) {
            state->flags &= ~GAME_FLAG_READY_FOR_UPDATE;
            state->game_timer = 0.0f;

            state->player.previous_position = state->player.position;

            if (wait) {
                state->flags &= ~GAME_FLAG_IS_MOVING;
            } else {
                if (input_key) {
                    state->flags &= ~GAME_FLAG_IS_MOVING;
                    IntX2 requested_cell = state->player.previous_position;
                    switch (input_key) {
                    case KEY_UP: requested_cell.y--; break;
                    case KEY_LEFT: requested_cell.x--; break;
                    case KEY_DOWN: requested_cell.y++; break;
                    case KEY_RIGHT: requested_cell.x++; break;
                    }
                    if (is_cell_valid(state, requested_cell, CELL_FLAG_WALKABLE)) {
                        state->player.position = requested_cell;
                    }
                    input_key = 0;
                } else {
                    state->player.position = astar_path(state, state->player.previous_position, state->mouse_target, CELL_FLAG_PLAYER_WALKABLE);
                    if (intX2_eq(state->player.position, state->mouse_target)) {
                        state->flags &= ~GAME_FLAG_IS_MOVING;
                    }
                }
                state->player.direction = get_direction(state->player.previous_position, state->player.position);
                discover_visible_cells(state);
            }

            for (int i = 0; i < CREATURE_CAPACITY; i++) {
                Creature *c = &state->creatures[i];
                c->previous_position = c->position;
                IntX2 old_pos = c->previous_position;
                state->grid[old_pos.x][old_pos.y] &= ~CELL_FLAG_CREATURE;
                IntX2 new_pos = old_pos;
                switch (c->type) {
                case CREATURE_DIGGER: {
                    new_pos = random_wander(state, old_pos, c->direction);
                } break;
                case CREATURE_EVIL_TRIANGLE: {
                    CoordAndDirection cad = bounce_path(state, old_pos, c->direction);
                    new_pos = cad.coord;
                    c->direction = cad.direction;
                } break;
                case CREATURE_BIG_EVIL_TRIANGLE: {
                    if (intX2_neq(c->last_known_player_location, INVALID_CELL)) {
                        new_pos = astar_path(state, old_pos, c->last_known_player_location, CELL_FLAG_CREATURE_WALKABLE);
                        if (intX2_eq(new_pos, old_pos)) {
                            c->last_known_player_location = INVALID_CELL;
                        }
                    }
                    if (intX2_eq(new_pos, old_pos)) {
                        new_pos = random_wander(state, old_pos, c->direction);
                    }

                    bool can_see_player = has_flag(state->grid[new_pos.x][new_pos.y], CELL_FLAG_VISIBLE);
                    if (can_see_player) {
                        c->last_known_player_location = state->player.position;
                    }
                    c->direction = get_direction(old_pos, new_pos);
                } break;
                }
                c->position = new_pos;
                state->grid[new_pos.x][new_pos.y] |= CELL_FLAG_CREATURE;
                bool creature_visible = has_flag(c->flags, CREATURE_FLAG_VISIBLE);
                bool cell_visible = has_flag(state->grid[c->position.x][c->position.y], CELL_FLAG_VISIBLE);
                if (!creature_visible && cell_visible) {
                    c->flags |= (CREATURE_FLAG_DISCOVERED | CREATURE_FLAG_VISIBLE);
                    state->flags &= ~GAME_FLAG_IS_MOVING;
                }
            }
        }

        render(state);
    }

    CloseWindow();

    free(state);

    return 0;
}