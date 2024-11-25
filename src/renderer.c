#include "../raylib/include/raylib.h"
#include "main.h"

void draw_cell(State *state, IntX2 cell, Color color) {
    IntX2 local_position = get_cell_local_position(state, cell);
    Rectangle rec = {
        .x = local_position.x * CELLSIZE,
        .y = local_position.y * CELLSIZE,
        .width = CELLSIZE,
        .height = CELLSIZE
    };
    DrawRectangleRec(rec, color);
}

void draw_evil_triangle(State *state, IntX2 center, float size, Color color) {
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

    IntX2 current = get_cell_local_position(state, c->position);
    IntX2 previous = get_cell_local_position(state, c->previous_position);

    IntX2 body = {
        (previous.x * CELLSIZE) + ((current.x - previous.x) * relative_time) + half_cell,
        (previous.y * CELLSIZE) + ((current.y - previous.y) * relative_time) + half_cell,
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
    IntX2 pupil = {0,0};
    int off = eye_radius * 0.25f;
    if (diagonal_eye) {
        switch (c->direction) {
        case DIAGONAL_NE: pupil.x = off; pupil.y = -off; break;
        case DIAGONAL_NW: pupil.x = -off; pupil.y = -off; break;
        case DIAGONAL_SW: pupil.x = -off; pupil.y = off; break;
        case DIAGONAL_SE: pupil.x = off; pupil.y = off; break;
        }
    } else {
        switch (c->direction) {
        case ORTHAGONAL_N: pupil.y = -off; break;
        case ORTHAGONAL_W: pupil.x = -off; break;
        case ORTHAGONAL_S: pupil.y = off; break;
        case ORTHAGONAL_E: pupil.x = off; break;
        }
    }

    DrawCircle(body.x + pupil.x + pupil.x, body.y + pupil.y + pupil.y, pupil_radius, BLACK);
}

void render(State *state) {
    BeginDrawing();

    ClearBackground(COLOR_UNDISCOVERED);

    IntX2 min = {
        (state->game_offset.x >= 0) ? state->game_offset.x : 0,
        (state->game_offset.y >= 0) ? state->game_offset.y : 0
    };
    IntX2 game_max = {
        state->game_offset.x + GAME_WIDTH,
        state->game_offset.y + GAME_HEIGHT
    };
    IntX2 max = {
        (game_max.x < GRID_WIDTH) ? game_max.x : GRID_WIDTH,
        (game_max.y < GRID_HEIGHT) ? game_max.y : GRID_HEIGHT,
    };
    for (int x = min.x; x < max.x; x++) {
        for (int y = min.y; y < max.y; y++) {
            int cell = state->grid[x][y];
            if (has_flag(cell, CELL_FLAG_DISCOVERED)) {
                bool visible = has_flag(cell, CELL_FLAG_VISIBLE);
                bool wall = has_flag(cell, CELL_FLAG_WALL);
                Color color;
                if (visible) {
                    color = wall ? COLOR_WALL_VISIBLE : COLOR_GROUND_VISIBLE;
                } else {
                    color = wall ? COLOR_WALL_INVISIBLE : COLOR_GROUND_INVISIBLE;
                }
                draw_cell(state, (IntX2) { x, y }, color);
            }
        }
    }

    IntX2 player_path = astar_path(state, state->player.position, state->mouse_current, CELL_FLAG_PLAYER_WALKABLE);
    bool player_path_found = intX2_neq(player_path, state->player.position);
    bool mouse_cell_discovered = !has_flag(state->grid[state->mouse_current.x][state->mouse_current.y], CELL_FLAG_DISCOVERED);
    if (!player_path_found || mouse_cell_discovered) {
        draw_cell(state, state->mouse_current, RED);
    } else {
        if (!has_flag(state->flags, GAME_FLAG_IS_MOVING)) {
            draw_cell(state, state->player.position, COLOR_PLAYER_PATH);
            draw_cell(state, player_path, COLOR_PLAYER_PATH);
        }
        while (!has_flag(state->flags, GAME_FLAG_IS_MOVING) && intX2_neq(player_path, state->mouse_current)) {
            player_path = astar_path(state, player_path, state->mouse_current, CELL_FLAG_PLAYER_WALKABLE);
            draw_cell(state, player_path, COLOR_PLAYER_PATH);
        }
    }

    draw_creature(state, &state->player);

    for (int i = 0; i < CREATURE_CAPACITY; i++) {
        Creature *c = &state->creatures[i];
        if (!has_flag(c->flags, CREATURE_FLAG_VISIBLE) ||
            c->position.x < state->game_offset.x ||
            c->position.x > state->game_offset.x + GAME_WIDTH ||
            c->position.y < state->game_offset.y ||
            c->position.y > state->game_offset.y + GAME_HEIGHT
        ) {
            continue;
        }
        draw_creature(state, c);
    }

    EndDrawing();
}