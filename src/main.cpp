#include "../raylib/include/raylib.h"
#include "main.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

bool has_bit(int flags, int flag) {
    return (flags & flag) == flag;
}

void setup_visibility(State *state) {
    const char *visibility_sphere_string =
    ".....00000....."
    "...00.....00..."
    "..0.........0.."
    ".0...........0."
    ".0...........0."
    "0.............0"
    "0.............0"
    "0.............0"
    "0.............0"
    "0.............0"
    ".0...........0."
    ".0...........0."
    "..0.........0.."
    "...00.....00..."
    ".....00000.....";
    const int radius = 7;
    const int diameter = 15;
    int cell_idx = 0;
    for (int y = 0; y < diameter; y++) {
        for (int x = 0; x < diameter; x++) {
            if (visibility_sphere_string[(y * diameter) + x] == '0') {
                state->visibility_sphere[cell_idx] = {x - radius, y - radius};
                cell_idx++;
            }
        }
    }
    if (cell_idx > VISIBILITY_SPHERE_CELL_AMOUNT) {
        exit(1);
    }
}

void update_visibility(State *state) {
    Coord a = {
        state->player.position.x,
        state->player.position.y,
    };
    const float resolution = 15;
    for (int i = 0; i < VISIBILITY_SPHERE_CELL_AMOUNT; i++) {
        Coord b = {
            (state->player.position.x + state->visibility_sphere[i].x),
            (state->player.position.y + state->visibility_sphere[i].y),
        };
        for (int j = 0; j < resolution; j++) {
            float fract = (float)j / resolution;
            Coord c = {
                a.x + ((b.x - a.x) * fract),
                a.y + ((b.y - a.y) * fract)
            };
            if (c.x < 0 || c.x >= GRID_WIDTH ||
                c.y < 0 || c.y >= GRID_HEIGHT
            ) {
                break;
            }
            uint8 *flags = &state->grid[c.x][c.y];
            *flags |= CELL_BIT_VISIBLE;
            if (has_bit(*flags, CELL_BIT_WALL)) {
                break;
            }
        }
    }
}

Coord cell_in_direction(Coord position, uint8 direction) {
    switch (direction) {
    case ORTHAGONAL_N: return (Coord) { position.x, position.y - 1 };
    case ORTHAGONAL_W: return (Coord) { position.x - 1, position.y };
    case ORTHAGONAL_S: return (Coord) { position.x, position.y + 1 };
    case ORTHAGONAL_E: return (Coord) { position.x + 1, position.y };
    default: return position;
    }
}

uint8 get_direction(Coord from, Coord to) {
    if (from.y > to.y) {
        return ORTHAGONAL_N;
    } else if (from.x > to.x) {
        return ORTHAGONAL_W;
    } else if (from.y < to.y) {
        return ORTHAGONAL_S;
    } else if (from.x < to.x) {
        return ORTHAGONAL_E;
    }
}

bool valid_cell(State *state, Coord position, int cell_bits) {
    return (
        position.x >= 0 && position.x < GRID_WIDTH &&
        position.y >= 0 && position.y < GRID_HEIGHT &&
        (state->grid[position.x][position.y] & cell_bits) == cell_bits
    );
}

void fill_cell(State *state, Coord position) {
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

void print_a_star(AStar *a, Coord start, Coord goal, int (* operation)(ANode *)) {
    printf("* * * * * * * * * * * *\n");
    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            ANode *n = &a->all_list[x][y];
            int something = operation(n);
            if (n->position == start) {
                printf("<@> ");
            } else if (n->position == goal) {
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

bool can_see_cell(State *state, Coord from, Coord to) {
    const Coord diff = {
        to.x - from.x,
        to.y - from.y
    };
    const float resolution = (abs(diff.x) + abs(diff.y)) * 2.0f;
    for (int i = 0; i < resolution; i++) {
        float fract = (float)i / resolution;
        Coord c = {
            from.x + (diff.x * fract),
            from.y + (diff.y * fract)
        };
        if (has_bit(state->grid[c.x][c.y], CELL_BIT_WALL)) {
            return false;
        }
    }
    return true;
}

Coord astar_path(State *state, Coord start, Coord goal) {
    if (start == goal) {
        return start;
    }

    AStar *a = &state->a_star;
    for (int x = 0; x < GRID_WIDTH; x++) {
        for (int y = 0; y < GRID_HEIGHT; y++) {
            a->all_list[x][y].position.x = x;
            a->all_list[x][y].position.y = y;
            a->all_list[x][y].g_cost = INT_MAX;
            a->all_list[x][y].f_cost = INT_MAX;
            a->all_list[x][y].came_from = 0;
        }
    }

    a->open_list_count = 1;
    a->open_list[0] = &(a->all_list[start.x][start.y]);
    a->open_list[0]->g_cost = 0;
    a->open_list[0]->f_cost = abs(start.x - goal.x) + abs(start.y - goal.y);

    a->closed_list_count = 0;

    while (a->open_list_count > 0) {
        int current_idx = 0;
        for (int i = 1; i < a->open_list_count; i++) {
            int other_cost = a->open_list[i]->f_cost;
            int current_cost = a->open_list[current_idx]->f_cost;
            if (other_cost < current_cost) {
                current_idx = i;
            }/*else if (other_cost == current_cost && GetRandomValue(0, 1) == 0) {
                current_idx = i;
            }*/
        }
        ANode *current = a->open_list[current_idx];
        if (current->position.x == goal.x && current->position.y == goal.y) {
            while (current->came_from->position != start) {
                current = current->came_from;
            }
            return current->position;
        }

        a->closed_list[a->closed_list_count] = current;
        a->closed_list_count++;

        a->open_list_count--;
        for (int i = current_idx; i < a->open_list_count; i++) {
            a->open_list[i] = a->open_list[i + 1];
        }

        int neighbour_count = 4;
        Coord neighbours[neighbour_count] = {
            { .x = current->position.x,     .y = current->position.y + 1    },
            { .x = current->position.x - 1, .y = current->position.y        },
            { .x = current->position.x,     .y = current->position.y - 1    },
            { .x = current->position.x + 1, .y = current->position.y        },
        };

        for (int i = 0; i < neighbour_count; i++) {
            if (!valid_cell(state, neighbours[i], (CELL_BIT_WALKABLE | CELL_BIT_VISIBLE))) {
                continue;
            }
            ANode *n = &(a->all_list[neighbours[i].x][neighbours[i].y]);
            int tentative_cost = current->g_cost + 1;
            if (tentative_cost < n->g_cost) {
                n->came_from = current;
                n->g_cost = tentative_cost;
                n->f_cost = n->g_cost + abs(n->position.x - goal.x) + abs(n->position.y - goal.y);
                bool neighbour_in_closed_list = false;
                for (int i = 0; i < a->closed_list_count; i++) {
                    if (a->closed_list[i]->position == n->position) {
                        neighbour_in_closed_list = true;
                        break;
                    }
                }
                if (neighbour_in_closed_list) {
                    continue;
                }
                bool neighbour_in_open_list = false;
                for (int i = 0; i < a->open_list_count; i++) {
                    if (a->open_list[i]->position == n->position) {
                        neighbour_in_open_list = true;
                        break;
                    }
                }
                if (neighbour_in_open_list) {
                    continue;
                }
                a->open_list[a->open_list_count] = n;
                a->open_list_count++;
            }
        }
    }

    return start;
}

CoordAndDirection bounce_path(State *state, Coord start, uint8 direction) {
    Coord n = {start.x, start.y - 1};
    Coord w = {start.x - 1, start.y};
    Coord s = {start.x, start.y + 1};
    Coord e = {start.x + 1, start.y};

    bool n_wall = !valid_cell(state, n, CELL_BIT_WALKABLE);
    bool w_wall = !valid_cell(state, w, CELL_BIT_WALKABLE);
    bool s_wall = !valid_cell(state, s, CELL_BIT_WALKABLE);
    bool e_wall = !valid_cell(state, e, CELL_BIT_WALKABLE);

    Coord ne = {start.x + 1, start.y - 1};
    Coord nw = {start.x - 1, start.y - 1};
    Coord sw = {start.x - 1, start.y + 1};
    Coord se = {start.x + 1, start.y + 1};

    bool ne_open = valid_cell(state, ne, CELL_BIT_WALKABLE);
    bool nw_open = valid_cell(state, nw, CELL_BIT_WALKABLE);
    bool sw_open = valid_cell(state, sw, CELL_BIT_WALKABLE);
    bool se_open = valid_cell(state, se, CELL_BIT_WALKABLE);

    int bounce = -1;

    switch (direction) {
        case DIAGONAL_NE:
            if (e_wall) {
                if (n_wall) { if (sw_open) bounce = DIAGONAL_SW; }
                else if (nw_open) bounce = DIAGONAL_NW;
                else bounce = DIAGONAL_SW;
            } else if (n_wall && se_open) bounce = DIAGONAL_SE;
            else if (!n_wall && ne_open) bounce = DIAGONAL_NE;
            else bounce = DIAGONAL_SW;
            break;

        case DIAGONAL_NW:
            if (w_wall) {
                if (n_wall) { if (se_open) bounce = DIAGONAL_SE; }
                else if (ne_open) bounce = DIAGONAL_NE;
                else bounce = DIAGONAL_SE;
            } else if (n_wall && sw_open) bounce = DIAGONAL_SW;
            else if (!n_wall && nw_open) bounce = DIAGONAL_NW;
            else bounce = DIAGONAL_SE;
            break;

        case DIAGONAL_SW:
            if (w_wall) {
                if (s_wall) { if (ne_open) bounce = DIAGONAL_NE; }
                else if (se_open) bounce = DIAGONAL_SE;
                else bounce = DIAGONAL_NE;
            } else if (s_wall && nw_open) bounce = DIAGONAL_NW;
            else if (!s_wall && sw_open) bounce = DIAGONAL_SW;
            else bounce = DIAGONAL_NE;
            break;

        case DIAGONAL_SE:
            if (e_wall) {
                if (s_wall) { if (nw_open) bounce = DIAGONAL_NW; }
                else if (sw_open) bounce = DIAGONAL_SW;
                else bounce = DIAGONAL_NW;
            } else if (s_wall && ne_open) bounce = DIAGONAL_NE;
            else if (!s_wall && se_open) bounce = DIAGONAL_SE;
            else bounce = DIAGONAL_NW;
            break;

        default:
            return (CoordAndDirection){start, 0};
    }

    if (bounce < 0) return (CoordAndDirection) { start, direction };

    switch (bounce) {
        case DIAGONAL_NE: return (CoordAndDirection){ne, DIAGONAL_NE};
        case DIAGONAL_NW: return (CoordAndDirection){nw, DIAGONAL_NW};
        case DIAGONAL_SE: return (CoordAndDirection){se, DIAGONAL_SE};
        case DIAGONAL_SW: return (CoordAndDirection){sw, DIAGONAL_SW};
    }
}

Coord random_wander(State *state, Coord start, uint8 direction) {
    int random = GetRandomValue(0, 3);
    for (int i = 0; i < 4; i++) {
        Coord position;
        switch (random) {
        case 0: position = (Coord) {start.x, start.y - 1}; break;
        case 1: position = (Coord) {start.x - 1, start.y}; break;
        case 2: position = (Coord) {start.x, start.y + 1}; break;
        case 3: position = (Coord) {start.x + 1, start.y}; break;
        }
        bool valid = valid_cell(state, position, CELL_BIT_WALKABLE);
        bool backtrack = cell_in_direction(start, (direction + 2) % 4) == position;
        if (valid && !backtrack) {
            return position;
        }
        random = (random + 1) % 4;
    }
    Coord backtrack_cell = cell_in_direction(start, (direction + 2) % 4);
    bool okay_backtrack_if_that_is_the_only_way = valid_cell(state, backtrack_cell, CELL_BIT_WALKABLE);
    if (okay_backtrack_if_that_is_the_only_way) {
        return backtrack_cell;
    }
    return start;
}

Coord random_dumb_path(State *state, Coord start, Coord goal, CellBits walk_bits) {
    Coord options[3];
    if (start.y > goal.y) {
        options[0] = { start.x, start.y - 1 };
        options[1] = { start.x - 1, start.y };
        options[2] = { start.x + 1, start.y };
    } else if (start.x > goal.x) {
        options[0] = { start.x, start.y - 1 };
        options[1] = { start.x - 1, start.y };
        options[2] = { start.x, start.y + 1 };
    } else if (start.y < goal.y) {
        options[0] = { start.x - 1, start.y };
        options[1] = { start.x, start.y + 1 };
        options[2] = { start.x + 1, start.y };
    } else if (start.x < goal.x) {
        options[0] = { start.x, start.y - 1 };
        options[1] = { start.x, start.y + 1 };
        options[2] = { start.x + 1, start.y };
    } else {
        return start;
    }
    int option_idx = GetRandomValue(0, 2);
    while (!valid_cell(state, options[option_idx], walk_bits)) {
        option_idx = (option_idx + 1) % 3;
    }
    return options[option_idx];
}

void draw_cell(Coord cell, Color color) {
    Rectangle rec = {
        .x = cell.x * CELLSIZE,
        .y = cell.y * CELLSIZE,
        .width = CELLSIZE,
        .height = CELLSIZE
    };
    DrawRectangleRec(rec, color);
}

void draw_evil_triangle(State *state, Coord center, float size, Color color) {
    int half_cell = CELLSIZE / 2;
    const float d = (2.0f * PI) / 3.0f;
    const float time = state->animation_timer / TIME_PER_ANIMATION;
    Vector2 v[3];
    for (int i = 0; i < 3; i++) {
        float value = (d * i) + (time * d);
        v[i] = {
            .x = center.x + (sin(value) * half_cell * size),
            .y = center.y + (cos(value) * half_cell * size),
        };
    }
    DrawTriangle(v[0], v[1], v[2], color);
}

void draw_creature(State *state, Creature *c) {
    int half_cell = CELLSIZE / 2;
    int relative_time = (state->game_timer / TIME_PER_TURN) * CELLSIZE;
    Coord body = {
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

void generate_map(State *state) {
    for (int x = 0; x < GRID_WIDTH; x++) {
        for (int y = 0; y < GRID_HEIGHT; y++) {
            state->grid[x][y] = CELL_BIT_WALL;
        }
    }
    const int pivot_box_size = 3;
    const int pivot_amount = 5;
    {
        Coord center = {(GRID_WIDTH / 2), (GRID_HEIGHT / 2)};
        for (int x = center.x - 2; x < center.x + 2; x++) {
            for (int y = center.y - 2; y < center.y + 2; y++) {
                state->grid[x][y] = CELL_BIT_WALKABLE;
            }
        }
    }
    for (int x = 0; x < pivot_box_size; x++) {
        for (int y = 0; y < pivot_box_size; y++) {
            int x2 = GRID_WIDTH - pivot_box_size + x;
            int y2 = GRID_HEIGHT - pivot_box_size + y;
            state->grid[x][y] = CELL_BIT_WALKABLE;
            state->grid[x2][y] = CELL_BIT_WALKABLE;
            state->grid[x][y2] = CELL_BIT_WALKABLE;
            state->grid[x2][y2] = CELL_BIT_WALKABLE;
        }
    }
    Coord pivots[pivot_amount] = {
        { GRID_WIDTH / 2, GRID_HEIGHT / 2 },
        { 1, 1 },
        { GRID_WIDTH - 2, 1 },
        { 1, GRID_HEIGHT - 1 },
        { GRID_WIDTH - 2, GRID_HEIGHT - 1 },
    };
    for (int pivot_idx = 0; pivot_idx < pivot_amount - 1; pivot_idx++) {
        Coord pos = pivots[pivot_idx];
        for (int i = 0; i < 500; i++) {
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
            state->grid[pos.x][pos.y] = CELL_BIT_WALKABLE;
        }
        Coord goal = pivots[pivot_idx + 1];
        while (pos != goal) {
            pos = random_dumb_path(state, pos, goal, (CellBits)(CELL_BIT_ANY));
            state->grid[pos.x][pos.y] = CELL_BIT_WALKABLE;
        }
    }
    int used_pivots_bits = 0;
    {
        int random_idx = GetRandomValue(0, pivot_amount - 1);
        int pivot_bit = 1 << random_idx;
        used_pivots_bits |= pivot_bit;
        state->player.previous_position = pivots[random_idx];
        state->player.position = pivots[random_idx];
    }
    for (int i = 0; i < CREATURE_CAPACITY; i++) {
        int random_idx;
        int pivot_bit;
        do {
            random_idx = (random_idx + 1) % pivot_amount;
            pivot_bit = 1 << random_idx;
        }
        while (has_bit(used_pivots_bits, pivot_bit));
        used_pivots_bits |= pivot_bit;
        state->creatures[i].previous_position = pivots[random_idx];
        state->creatures[i].position = pivots[random_idx];
    }
}

int main(void) {
    const int screen_width = CELLSIZE * GRID_WIDTH;
    const int screen_height = CELLSIZE * GRID_HEIGHT;

    InitWindow(screen_width, screen_height, "Elvira");

    SetTargetFPS(60);

    State *state = (State *)calloc(1, sizeof(State));

    state->player = (Creature) {
        .type = CREATURE_PLAYER,
    };

    state->creatures[0] = {
        .type = CREATURE_EVIL_TRIANGLE,
        .direction = DIAGONAL_NE,
    };
    state->creatures[1] = {
        .type = CREATURE_BIG_EVIL_TRIANGLE,
    };

    setup_visibility(state);

    generate_map(state);

    update_visibility(state);

    bool ready_for_update = false;
    bool is_moving = false;
    Coord debug_cells[2] = { { 0, 0 }, { GRID_WIDTH - 1, GRID_HEIGHT - 1 } };

    while (!WindowShouldClose()) {
        Vector2 mouse_vector2 = GetMousePosition();
        Coord mouse_frame_coord = {
            .x = mouse_vector2.x / CELLSIZE,
            .y = mouse_vector2.y / CELLSIZE,
        };
        Rectangle mouse_rec = {
            .x = mouse_frame_coord.x * CELLSIZE,
            .y = mouse_frame_coord.y * CELLSIZE,
            CELLSIZE,
            CELLSIZE
        };

        bool left_mouse_pressed = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
        bool mouse_condition = left_mouse_pressed && astar_path(state, state->player.position, mouse_frame_coord) != state->player.position;

        if (mouse_condition) {
            state->mouse_target_coord = mouse_frame_coord;
        }

        bool wait = IsKeyPressed(KEY_SPACE);
        if (mouse_condition || wait) {
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
                state->player.position = astar_path(state, state->player.previous_position, state->mouse_target_coord);
                if (state->player.position == state->mouse_target_coord) {
                    is_moving = false;
                }
                state->player.direction = get_direction(state->player.previous_position, state->player.position);
                update_visibility(state);
            }

            for (int i = 0; i < CREATURE_CAPACITY; i++) {
                Creature *c = &state->creatures[i];
                c->previous_position = c->position;
                Coord old_pos = c->previous_position;
                state->grid[old_pos.x][old_pos.y] &= ~CELL_BIT_CREATURE;
                Coord new_pos;
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
                    new_pos = old_pos;
                    if (can_see_cell(state, old_pos, state->player.previous_position)) {
                        new_pos = astar_path(state, old_pos, state->player.previous_position);
                        debug_cells[0] = old_pos;
                        debug_cells[1] = state->player.previous_position;
                    }
                    if (new_pos == old_pos) {
                        new_pos = random_wander(state, old_pos, c->direction);
                    }
                    c->direction = get_direction(old_pos, new_pos);
                } break;
                }
                c->position = new_pos;
                state->grid[new_pos.x][new_pos.y] |= CELL_BIT_CREATURE;
                bool creature_visible = has_bit(c->bits, CREATURE_BIT_VISIBLE);
                bool cell_visible = has_bit(state->grid[c->position.x][c->position.y], CELL_BIT_VISIBLE);
                if (!creature_visible && cell_visible) {
                    c->bits |= CREATURE_BIT_VISIBLE;
                    is_moving = false;
                }
            }
        }

        BeginDrawing();

        ClearBackground(COLOR_UNDISCOVERED);

        for (int x = 0; x < GRID_WIDTH; x++) {
            for (int y = 0; y < GRID_HEIGHT; y++) {
                int cell = state->grid[x][y];
                if (has_bit(cell, CELL_BIT_VISIBLE)) {
                    if (has_bit(cell, CELL_BIT_WALL)) {
                        draw_cell({x,y}, COLOR_WALL);
                    } else {
                        draw_cell({x,y}, COLOR_EMPTY);
                    }
                }
            }
        }

        Coord player_path = astar_path(state, state->player.position, mouse_frame_coord);
        if (player_path == state->player.position || !has_bit(state->grid[mouse_frame_coord.x][mouse_frame_coord.y], CELL_BIT_VISIBLE)) {
            draw_cell(mouse_frame_coord, RED);
        } else {
            if (!is_moving) {
                draw_cell(state->player.position, COLOR_PLAYER_PATH);
                draw_cell(player_path, COLOR_PLAYER_PATH);
            }
            while (!is_moving && player_path != mouse_frame_coord) {
                player_path = astar_path(state, player_path, mouse_frame_coord);
                draw_cell(player_path, COLOR_PLAYER_PATH);
            }
        }

        draw_cell(debug_cells[0], PURPLE);
        draw_cell(debug_cells[1], YELLOW);

        draw_creature(state, &state->player);

        for (int i = 0; i < CREATURE_CAPACITY; i++) {
            Creature *c = &state->creatures[i];
            if (!has_bit(c->bits, CREATURE_BIT_VISIBLE)) {
                continue;
            }
            draw_creature(state, c);
        }

        EndDrawing();
    }

    CloseWindow();

    free(state);

    return 0;
}