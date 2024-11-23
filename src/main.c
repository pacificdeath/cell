#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "../raylib/include/raylib.h"

#include "main.h"
#include "map.c"
#include "vision.c"
#include "movement.c"

uint8 get_direction(Cell from, Cell to) {
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

void fill_cell(State *state, Cell position) {
    state->grid[position.x][position.y] &= ~CELL_BIT_WALKABLE;
    state->grid[position.x][position.y] |= CELL_BIT_WALKABLE;
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

void print_a_star(AStar *a, Cell start, Cell goal, int (* operation)(ANode *)) {
    printf("* * * * * * * * * * * *\n");
    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            ANode *n = &a->all_list[x][y];
            int something = operation(n);
            if (cell_eq(n->position, start)) {
                printf("<@> ");
            } else if (cell_eq(n->position, goal)) {
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

void draw_cell(Cell cell, Color color) {
    Rectangle rec = {
        .x = cell.x * CELLSIZE,
        .y = cell.y * CELLSIZE,
        .width = CELLSIZE,
        .height = CELLSIZE
    };
    DrawRectangleRec(rec, color);
}

void draw_evil_triangle(State *state, Cell center, float size, Color color) {
    int half_cell = CELLSIZE / 2;
    const float d = (2.0f * PI) / 3.0f;
    const float time = state->animation_timer / TIME_PER_ANIMATION;
    Vector2 v[3];
    for (int i = 0; i < 3; i++) {
        float value = (d * i) + (time * d);
        v[i] = (Vector2) {
            .x = center.x + (sin(value) * half_cell * size),
            .y = center.y + (cos(value) * half_cell * size),
        };
    }
    DrawTriangle(v[0], v[1], v[2], color);
}

void draw_creature(State *state, Creature *c) {
    int half_cell = CELLSIZE / 2;
    int relative_time = (state->game_timer / TIME_PER_TURN) * CELLSIZE;
    Cell body = {
        (c->previous_position.x * CELLSIZE) + ((c->position.x - c->previous_position.x) * relative_time) + half_cell,
        (c->previous_position.y * CELLSIZE) + ((c->position.y - c->previous_position.y) * relative_time) + half_cell,
    };

    int eye_radius;
    bool diagonal_eye = false;
    switch (c->type) {
    case CREATURE_PLAYER: {
        DrawCircle(body.x, body.y, half_cell, COLOR_CREATURE_PLAYER);
        eye_radius = 0.3f * CELLSIZE;
    } break;
    case CREATURE_DIGGER: {
        Rectangle rec = { body.x, body.y, CELLSIZE, CELLSIZE };
        Vector2 origin = { half_cell, half_cell };
        DrawRectanglePro(rec, origin, 0.0f, GREEN);
    } break;
    case CREATURE_EVIL_TRIANGLE: {
        draw_evil_triangle(state, body, 1.0f, COLOR_CREATURE_ENEMY);
        eye_radius = 0.2f * CELLSIZE;
        diagonal_eye = true;
    } break;
    case CREATURE_BIG_EVIL_TRIANGLE: {
        draw_evil_triangle(state, body, 1.5f, COLOR_CREATURE_ENEMY2);
        eye_radius = 0.3f * CELLSIZE;
    } break;
    }

    DrawCircle(body.x, body.y, eye_radius, WHITE);

    int pupil_radius = eye_radius * 0.4f;
    int pupil_x = 0;
    int pupil_y = 0;
    int off = eye_radius * 0.25f;
    if (diagonal_eye) {
        switch (c->direction) {
        case DIAGONAL_NE: pupil_x = off; pupil_y = -off; break;
        case DIAGONAL_NW: pupil_x = -off; pupil_y = -off; break;
        case DIAGONAL_SW: pupil_x = -off; pupil_y = off; break;
        case DIAGONAL_SE: pupil_x = off; pupil_y = off; break;
        }
    } else {
        switch (c->direction) {
        case ORTHAGONAL_N: pupil_y = -off; break;
        case ORTHAGONAL_W: pupil_x = -off; break;
        case ORTHAGONAL_S: pupil_y = off; break;
        case ORTHAGONAL_E: pupil_x = off; break;
        }
    }

    DrawCircle(body.x + pupil_x + pupil_x, body.y + pupil_y + pupil_y, pupil_radius, BLACK);
}

int main(void) {
    const int screen_width = CELLSIZE * GRID_WIDTH;
    const int screen_height = CELLSIZE * GRID_HEIGHT;

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

    discover_visible_cells(state);

    bool ready_for_update = false;
    bool is_moving = false;
    int input_key = 0;
    Cell debug_cells[2] = { { 0, 0 }, { GRID_WIDTH - 1, GRID_HEIGHT - 1 } };

    while (!WindowShouldClose()) {
        Vector2 mouse_vector2 = GetMousePosition();
        Cell mouse_frame_coord = {
            .x = mouse_vector2.x / CELLSIZE,
            .y = mouse_vector2.y / CELLSIZE,
        };

        bool left_mouse_pressed = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
        bool mouse_condition = left_mouse_pressed && cell_neq(
            astar_path(state, state->player.position, mouse_frame_coord, PLAYER_WALKABLE),
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
            state->mouse_target_coord = mouse_frame_coord;
        }

        bool wait = IsKeyPressed(KEY_SPACE);
        if (mouse_condition || input_key || wait) {
            is_moving = true;
        }

        float frame_time = GetFrameTime();
        state->animation_timer += frame_time;
        if (state->animation_timer >= TIME_PER_ANIMATION) {
            state->animation_timer -= TIME_PER_ANIMATION;
        }

        if (state->game_timer < TIME_PER_TURN) {
            state->game_timer += frame_time;
        } else if (state->game_timer > TIME_PER_TURN && !ready_for_update) {
            state->game_timer = TIME_PER_TURN;
            ready_for_update = true;
            for (int i = 0; i < CREATURE_CAPACITY; i++) {
                Creature *c = &state->creatures[i];
                bool creature_visible = has_bit(c->bits, CREATURE_BIT_VISIBLE);
                bool cell_visible = has_bit(state->grid[c->position.x][c->position.y], CELL_BIT_VISIBLE);
                if (creature_visible && !cell_visible) {
                    c->bits &= ~CREATURE_BIT_VISIBLE;
                }
            }
        }

        if (ready_for_update && is_moving) {
            ready_for_update = false;
            state->game_timer = 0.0f;

            state->player.previous_position = state->player.position;

            if (wait) {
                is_moving = false;
            } else {
                if (input_key) {
                    is_moving = false;
                    Cell requested_cell = state->player.previous_position;
                    switch (input_key) {
                    case KEY_UP: requested_cell.y--; break;
                    case KEY_LEFT: requested_cell.x--; break;
                    case KEY_DOWN: requested_cell.y++; break;
                    case KEY_RIGHT: requested_cell.x++; break;
                    }
                    if (is_valid_cell(state, requested_cell, CELL_BIT_WALKABLE)) {
                        state->player.position = requested_cell;
                    }
                    input_key = 0;
                } else {
                    state->player.position = astar_path(state, state->player.previous_position, state->mouse_target_coord, PLAYER_WALKABLE);
                    if (cell_eq(state->player.position, state->mouse_target_coord)) {
                        is_moving = false;
                    }
                }
                state->player.direction = get_direction(state->player.previous_position, state->player.position);
                discover_visible_cells(state);
            }

            for (int i = 0; i < CREATURE_CAPACITY; i++) {
                Creature *c = &state->creatures[i];
                c->previous_position = c->position;
                Cell old_pos = c->previous_position;
                state->grid[old_pos.x][old_pos.y] &= ~CELL_BIT_CREATURE;
                Cell new_pos = old_pos;
                switch (c->type) {
                case CREATURE_PLAYER: {
                } break;
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
                        new_pos = astar_path(state, old_pos, c->last_known_player_location, CREATURE_WALKABLE);
                        if (cell_eq(new_pos, old_pos)) {
                            c->last_known_player_location = INVALID_CELL;
                        }
                    }
                    if (cell_eq(new_pos, old_pos)) {
                        new_pos = random_wander(state, old_pos, c->direction);
                    }

                    bool can_see_player = has_bit(state->grid[new_pos.x][new_pos.y], CELL_BIT_VISIBLE);
                    if (can_see_player) {
                        c->last_known_player_location = state->player.position;
                        debug_cells[0] = new_pos;
                        debug_cells[1] = state->player.position;
                    }
                    c->direction = get_direction(old_pos, new_pos);
                } break;
                }
                c->position = new_pos;
                state->grid[new_pos.x][new_pos.y] |= CELL_BIT_CREATURE;
                bool creature_visible = has_bit(c->bits, CREATURE_BIT_VISIBLE);
                bool cell_visible = has_bit(state->grid[c->position.x][c->position.y], CELL_BIT_VISIBLE);
                if (!creature_visible && cell_visible) {
                    c->bits |= (CREATURE_BIT_DISCOVERED | CREATURE_BIT_VISIBLE);
                    is_moving = false;
                }
            }
        }

        BeginDrawing();

        ClearBackground(COLOR_UNDISCOVERED);

        for (int x = 0; x < GRID_WIDTH; x++) {
            for (int y = 0; y < GRID_HEIGHT; y++) {
                int cell = state->grid[x][y];
                if (has_bit(cell, CELL_BIT_DISCOVERED)) {
                    bool visible = has_bit(cell, CELL_BIT_VISIBLE);
                    bool wall = has_bit(cell, CELL_BIT_WALL);
                    Color color;
                    if (visible) {
                        color = wall ? COLOR_WALL_VISIBLE : COLOR_GROUND_VISIBLE;
                    } else {
                        color = wall ? COLOR_WALL_INVISIBLE : COLOR_GROUND_INVISIBLE;
                    }
                    draw_cell((Cell) { x, y }, color);
                }
            }
        }

        Cell player_path = astar_path(state, state->player.position, mouse_frame_coord, PLAYER_WALKABLE);
        bool player_path_found = cell_neq(player_path, state->player.position);
        bool mouse_cell_discovered = !has_bit(state->grid[mouse_frame_coord.x][mouse_frame_coord.y], CELL_BIT_DISCOVERED);
        if (!player_path_found || mouse_cell_discovered) {
            draw_cell(mouse_frame_coord, RED);
        } else {
            if (!is_moving) {
                draw_cell(state->player.position, COLOR_PLAYER_PATH);
                draw_cell(player_path, COLOR_PLAYER_PATH);
            }
            while (!is_moving && cell_neq(player_path, mouse_frame_coord)) {
                player_path = astar_path(state, player_path, mouse_frame_coord, PLAYER_WALKABLE);
                draw_cell(player_path, COLOR_PLAYER_PATH);
            }
        }

        draw_cell(debug_cells[0], PURPLE);
        draw_cell(debug_cells[1], YELLOW);

        draw_creature(state, &state->player);

        for (int i = 0; i < CREATURE_CAPACITY; i++) {
            Creature *c = &state->creatures[i];
            if (!has_bit(c->bits, CREATURE_BIT_VISIBLE)) {
                //continue;
            }
            draw_creature(state, c);
        }

        EndDrawing();
    }

    CloseWindow();

    free(state);

    return 0;
}