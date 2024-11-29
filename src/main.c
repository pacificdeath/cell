#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "../raylib/include/raylib.h"

#include "main.h"
#include "map.c"
#include "vision.c"
#include "movement.c"
#include "renderer.c"

void update_creature_direction(Creature *c) {
    if (c->previous_position.y > c->position.y) {
        c->direction = ORTHAGONAL_N;
    } else if (c->previous_position.x > c->position.x) {
        c->direction = ORTHAGONAL_W;
    } else if (c->previous_position.y < c->position.y) {
        c->direction = ORTHAGONAL_S;
    } else if (c->previous_position.x < c->position.x) {
        c->direction = ORTHAGONAL_E;
    }
}

void fill_cell(State *state, Cell position) {
    state->grid[position.x][position.y] &= ~CELL_FLAG_WALKABLE;
    state->grid[position.x][position.y] |= CELL_FLAG_WALKABLE;
}

void update_game_offset(State *state) {
    Cell local_dimensions = { GAME_WIDTH, GAME_HEIGHT };
    state->game_offset = cell_subtract(state->player.position, cell_divide(local_dimensions, 2));
}

int main(void) {
    const int screen_width = CELLSIZE * GAME_WIDTH;
    const int screen_height = CELLSIZE * GAME_HEIGHT;

    InitWindow(screen_width, screen_height, "Cell");

    SetTargetFPS(60);

    #if DEBUG
    SetRandomSeed(8);
    #endif

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
    bool player_arrow_key_move = false;
    bool map_view = false;

    while (!WindowShouldClose()) {
        float frame_time = GetFrameTime();
        state->mouse_current = screen_to_game_position(state, GetMousePosition());

        if (IsKeyPressed(KEY_M)) {
            map_view = !map_view;
        }

        if (map_view) {
            draw_map_only(state);
            continue;
        }

        bool left_mouse_pressed = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
        bool mouse_condition = left_mouse_pressed && cell_neq(
            astar_path(state, state->player.position, state->mouse_current, CELL_FLAG_PLAYER_WALKABLE),
            state->player.position
        );

        if (IsKeyPressed(KEY_UP)) {
            input_key = KEY_UP;
            player_arrow_key_move = true;
            state->key_repeat_timer = 0.0f;
        } else if (IsKeyPressed(KEY_LEFT)) {
            input_key = KEY_LEFT;
            player_arrow_key_move = true;
            state->key_repeat_timer = 0.0f;
        } else if (IsKeyPressed(KEY_DOWN)) {
            input_key = KEY_DOWN;
            player_arrow_key_move = true;
            state->key_repeat_timer = 0.0f;
        } else if (IsKeyPressed(KEY_RIGHT)) {
            input_key = KEY_RIGHT;
            player_arrow_key_move = true;
            state->key_repeat_timer = 0.0f;
        } else if (IsKeyDown(input_key)) {
            if (state->key_repeat_timer < KEY_REPEAT_THRESHOLD) {
                state->key_repeat_timer += frame_time;
            } else {
                player_arrow_key_move = true;
            }
        }

        if (mouse_condition) {
            state->mouse_target = state->mouse_current;
        }

        bool wait = IsKeyPressed(KEY_SPACE);
        if (mouse_condition || player_arrow_key_move || wait) {
            state->flags |= GAME_FLAG_IS_MOVING;
        }

        state->animation_timer += frame_time;
        if (state->animation_timer >= TIME_PER_ANIMATION) {
            state->animation_timer -= TIME_PER_ANIMATION;
        }

        if (state->game_timer < TIME_PER_TURN) {
            state->game_timer += frame_time;
        }
        if (state->game_timer > TIME_PER_TURN && !has_flag(state->flags, GAME_FLAG_READY_FOR_UPDATE)) {
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
                if (player_arrow_key_move) {
                    state->flags &= ~GAME_FLAG_IS_MOVING;
                    Cell requested_cell = state->player.previous_position;
                    switch (input_key) {
                    case KEY_UP: requested_cell.y--; break;
                    case KEY_LEFT: requested_cell.x--; break;
                    case KEY_DOWN: requested_cell.y++; break;
                    case KEY_RIGHT: requested_cell.x++; break;
                    }
                    if (is_cell_valid(state, requested_cell, CELL_FLAG_WALKABLE)) {
                        state->player.position = requested_cell;
                    }
                    player_arrow_key_move = false;
                } else {
                    state->player.position = astar_path(state, state->player.previous_position, state->mouse_target, CELL_FLAG_PLAYER_WALKABLE);
                    if (cell_eq(state->player.position, state->mouse_target)) {
                        state->flags &= ~GAME_FLAG_IS_MOVING;
                    }
                }
                update_creature_direction(&state->player);

                set_invisible(state);
                update_game_offset(state);
                discover_visible_cells(state);
            }

            for (int i = 0; i < CREATURE_CAPACITY; i++) {
                Creature *c = &state->creatures[i];
                c->previous_position = c->position;
                Cell old_pos = c->previous_position;
                state->grid[old_pos.x][old_pos.y] &= ~CELL_FLAG_CREATURE;
                Cell new_pos = old_pos;
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
                    if (cell_neq(c->last_known_player_location, INVALID_CELL)) {
                        new_pos = astar_path(state, old_pos, c->last_known_player_location, CELL_FLAG_CREATURE_WALKABLE);
                        if (cell_eq(new_pos, old_pos)) {
                            c->last_known_player_location = INVALID_CELL;
                        }
                    }
                    if (cell_eq(new_pos, old_pos)) {
                        new_pos = random_wander(state, old_pos, c->direction);
                    }

                    bool can_see_player = has_flag(state->grid[new_pos.x][new_pos.y], CELL_FLAG_VISIBLE);
                    if (can_see_player) {
                        c->last_known_player_location = state->player.position;
                    }
                    update_creature_direction(c);
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
        state->turn_time = (state->game_timer / TIME_PER_TURN) * CELLSIZE;

        render(state);
    }

    CloseWindow();

    free(state);

    return 0;
}