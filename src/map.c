#include "main.h"

static void dig_towards_target(State *state, Cell start, Cell goal, int walkable_flags, int width) {
    Cell cell = start;
    while (cell_neq(cell, goal)) {
        Cell options[3];
        if (cell.y > goal.y) {
            options[0] = (Cell) { cell.x, cell.y - 1 };
            options[1] = (Cell) { cell.x - 1, cell.y };
            options[2] = (Cell) { cell.x + 1, cell.y };
        } else if (cell.x > goal.x) {
            options[0] = (Cell) { cell.x, cell.y - 1 };
            options[1] = (Cell) { cell.x - 1, cell.y };
            options[2] = (Cell) { cell.x, cell.y + 1 };
        } else if (cell.y < goal.y) {
            options[0] = (Cell) { cell.x - 1, cell.y };
            options[1] = (Cell) { cell.x, cell.y + 1 };
            options[2] = (Cell) { cell.x + 1, cell.y };
        } else if (cell.x < goal.x) {
            options[0] = (Cell) { cell.x, cell.y - 1 };
            options[1] = (Cell) { cell.x, cell.y + 1 };
            options[2] = (Cell) { cell.x + 1, cell.y };
        }
        int option_idx = GetRandomValue(0, 2);
        while (!is_cell_valid(state, options[option_idx], CELL_FLAG_ANY)) {
            option_idx = (option_idx + 1) % 3;
        }
        cell = options[option_idx];
        int half_width = width / 2;
        for (int x = 0; x < width; x++) {
            for (int y = 0; y < width; y++) {
                Cell surrounding_cell = {
                    cell.x - half_width + x,
                    cell.y - half_width + y
                };
                if (is_cell_valid(state, surrounding_cell, CELL_FLAG_WALL)) {
                    state->grid[surrounding_cell.x][surrounding_cell.y] = CELL_FLAG_WALKABLE;
                }
            }
        }
    }
}

static void dig_randomly(State *state, Cell start, int walkable_flags) {
    Cell cell = start;
    state->grid[cell.x][cell.y] = CELL_FLAG_WALKABLE;
    for (int i = 0; i < 100; i++) {
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
}

int random_direction(State *state, int current_direction) {
    int random_direction;
    do {
        random_direction = GetRandomValue(0, 3);
    } while (random_direction == current_direction);
    return random_direction;
}

int perpendicular_direction(Cell position, int direction) {
    switch (direction) {
    case ORTHAGONAL_N:
    case ORTHAGONAL_S: {
        if (position.x > (GRID_WIDTH / 2)) {
            return ORTHAGONAL_W;
        }
        return ORTHAGONAL_E;
    }
    case ORTHAGONAL_W:
    case ORTHAGONAL_E: {
        if (position.y > (GRID_HEIGHT / 2)) {
            return ORTHAGONAL_N;
        }
        return ORTHAGONAL_S;
    }
    default: {
        return NO_DIRECTION;
    }
    }
}

inline static int get_horizontal_direction_away_from_closest_edge(int x) {
    return (x > (GRID_WIDTH / 2)) ? ORTHAGONAL_W : ORTHAGONAL_E;
}

inline static int get_vertical_direction_away_from_closest_edge(int y) {
    return (y > (GRID_HEIGHT / 2)) ? ORTHAGONAL_N : ORTHAGONAL_S;
}

int oppositie_direction(int direction) {
    return (direction + 1) % 4;
}

int is_horizontal(int direction) {
    return direction == ORTHAGONAL_W || direction == ORTHAGONAL_E;
}

int is_vertical(int direction) {
    return direction == ORTHAGONAL_N || direction == ORTHAGONAL_S;
}

int new_quadrant_disgusted_direction(Cell position, int direction) {
    int horizontal_option = get_horizontal_direction_away_from_closest_edge(position.x);
    int vertical_option = get_vertical_direction_away_from_closest_edge(position.y);
    if (is_horizontal(direction)) {
        return vertical_option;
    }
    if (is_vertical(direction)) {
        return horizontal_option;
    }
    return (GetRandomValue(0, 1) == 0) ? horizontal_option : vertical_option;
}

int quadrant_disgusted_direction(Cell position) {
    int horizontal_option = get_horizontal_direction_away_from_closest_edge(position.x);
    int vertical_option = get_vertical_direction_away_from_closest_edge(position.y);
    return (GetRandomValue(0, 1) == 0) ? horizontal_option : vertical_option;
}

Room get_space_in_direction(Cell from, int direction, int length, int thickness) {
    const int half_thickness = thickness / 2;
    Room room;

    switch (direction) {
    case ORTHAGONAL_N: {
        room.position = (Cell) {
            from.x - half_thickness,
            from.y - length,
        };
    } break;
    case ORTHAGONAL_S: {
        room.position = (Cell) {
            from.x - half_thickness,
            from.y + length,
        };
    } break;
    case ORTHAGONAL_W: {
        room.position = (Cell) {
            from.x - length,
            from.y - half_thickness,
        };
    } break;
    case ORTHAGONAL_E: {
        room.position = (Cell) {
            from.x + length,
            from.y - half_thickness,
        };
    } break;
    }

    switch (direction) {
    case ORTHAGONAL_N:
    case ORTHAGONAL_S: {
        room.size = (Cell) {
            thickness,
            length,
        };
    } break;
    case ORTHAGONAL_W:
    case ORTHAGONAL_E: {
        room.size = (Cell) {
            length,
            thickness,
        };
    } break;
    }

    return room;
}

bool room_has_flags(State *state, Room *room, int flags) {
    for (int x = 0; x < room->size.x; x++) {
        for (int y = 0; y < room->size.y; y++) {
            Cell cell = {
                room->position.x + x,
                room->position.y + y
            };
            if (has_flag(state->grid[cell.x][cell.y], flags)) {
                return true;
            }
        }
    }
    return false;
}

bool is_space_available(State *state, Room *room, int flags) {
    for (int x = 0; x < room->size.x; x++) {
        for (int y = 0; y < room->size.y; y++) {
            Cell cell = {
                room->position.x + x,
                room->position.y + y
            };
            if (!is_cell_valid(state, cell, flags)) {
                return false;
            }
        }
    }
    return true;
}

void room_set_flags(State *state, Room *room, int flags) {
    for (int x = 0; x < room->size.x; x++) {
        for (int y = 0; y < room->size.y; y++) {
            state->grid[room->position.x + x][room->position.y + y] = flags;
        }
    }
}

void tunneler_dig(State *state, Tunneler *tunneler) {
    const int width_with_padding = tunneler->width + (tunneler->padding * 2);

    {
        Cell current_position = tunneler->position;
        Room current_space;
        bool valid;
        do {
            current_space = get_space_in_direction(current_position, tunneler->direction, 1, tunneler->width);
            valid = is_space_available(state, &current_space, CELL_FLAG_ANY);
            current_position = get_cell_in_direction(current_position, tunneler->direction, 1);
        } while (valid && room_has_flags(state, &current_space, CELL_FLAG_WALKABLE));
        if (!valid) {
            return;
        }

        Room initial_required_space = get_space_in_direction(current_space.position, tunneler->direction, width_with_padding, width_with_padding);
        if (!is_space_available(state, &initial_required_space, CELL_FLAG_ANY)) {
            return;
        }
    }

    int r = GetRandomValue(tunneler->width * 5, tunneler->width * 20);
    for (int i = 0; i < r; i++) {
        Cell lookahead_cell = get_cell_in_direction(tunneler->position, tunneler->direction, 2);
        Room lookahead_room = get_space_in_direction(lookahead_cell, tunneler->direction, 1, width_with_padding);

        if (!is_space_available(state, &lookahead_room, CELL_FLAG_WALL)) {
            return;
        }

        Cell next_cell = get_cell_in_direction(tunneler->position, tunneler->direction, 1);
        Room next_space = get_space_in_direction(next_cell, tunneler->direction, 1, tunneler->width);

        room_set_flags(state, &next_space, CELL_FLAG_PLAYER_WALKABLE);
        tunneler->position = next_cell;
    }
}

Cell room_center(Room *room) {
    return (Cell) {
        room->position.x + (room->size.x / 2),
        room->position.y + (room->size.y / 2),
    };
}

int set_to_possible_direction(State *state, Tunneler *tunneler) {
    bool directions[4];
    bool some_found = false;
    for (int i = 0; i < 4; i++) {
        Room room = get_space_in_direction(tunneler->position, i, tunneler->width, tunneler->width);
        directions[i] = (i != tunneler->direction) && is_space_available(state, &room, CELL_FLAG_WALL);
        if (directions[i]) {
            some_found = true;
        }
    }
    if (!some_found) {
        return -1;
    }
    int direction;
    do {
        direction = GetRandomValue(0,3);
    } while (!directions[direction]);
    return direction;
}

void dig(State *state, Tunneler *tunneler) {
    if (0) {
        Room room;
        room.position = tunneler->position;
        int distance = 0;
        bool valid;
        do {
            room = get_space_in_direction(room.position, tunneler->direction, tunneler->width, tunneler->width);
            valid = is_space_available(state, &room, CELL_FLAG_ANY);
            distance++;
        } while (valid && room_has_flags(state, &room, CELL_FLAG_WALKABLE));
        if (!valid) {
            return;
        }
        tunneler->position = get_cell_in_direction(tunneler->position, tunneler->direction, distance);
    }
    for (int i = 0; i < tunneler->lifetime; i++) {
        Room room = get_space_in_direction(tunneler->position, tunneler->direction, tunneler->width, tunneler->width);
        Room padded_room = get_space_in_direction(tunneler->position, tunneler->direction, (tunneler->width + tunneler->padding), tunneler->width + (tunneler->padding * 2));
        if (!is_space_available(state, &padded_room, CELL_FLAG_ANY) ||
            GetRandomValue(0, 100 - tunneler->chance_to_turn) == 0
        ) {
            tunneler->direction = set_to_possible_direction(state, tunneler);
            if (tunneler->direction < 0) {
                return;
            }
            continue;
        }
        room_set_flags(state, &room, CELL_FLAG_WALKABLE);
        tunneler->position = (Cell)room_center(&room);
    }
}

bool try_add_random_room(State *state, Room *room_result) {
    const int min_room_size = 3;
    const int padding = 1;

    Cell position = {
        GetRandomValue(1, GRID_WIDTH - 2 - min_room_size - padding),
        GetRandomValue(1, GRID_HEIGHT - 2 - min_room_size - padding),
    };

    const Cell largest_possible_room_size = {
        GRID_WIDTH - position.x - (padding * 2),
        GRID_HEIGHT - position.y - (padding * 2),
    };

    const int max_room_size = 20;

    Cell size = {
        GetRandomValue(min_room_size, largest_possible_room_size.x < max_room_size
            ? largest_possible_room_size.x
            : max_room_size),
        GetRandomValue(min_room_size, largest_possible_room_size.y < max_room_size
            ? largest_possible_room_size.y
            : max_room_size),
    };

    Room padded_room = {
        .position = { position.x - 1, position.y - 1 },
        .size = { size.x + 2, size.y + 2, },
    };

    if (!is_space_available(state, &padded_room, CELL_FLAG_WALL)) {
        return false;
    }

    for (int x = 0; x < size.x; x++) {
        for (int y = 0; y < size.y; y++) {
            state->grid[position.x + x][position.y + y] = CELL_FLAG_WALKABLE;
        }
    }

    room_result->position = position;
    room_result->size = size;

    return true;
}

void gen_map(State *state) {
    for (int x = 0; x < GRID_WIDTH; x++) {
        for (int y = 0; y < GRID_HEIGHT; y++) {
            state->grid[x][y] = CELL_FLAG_WALL;
        }
    }

    MapGen *mapgen = (MapGen *)malloc(sizeof(MapGen));

    const int room_capacity = 10;
    int room_count = 0;

    {
        for (int i = 0; i < room_capacity; i++) {
            if (try_add_random_room(state, &mapgen->rooms[room_count])) {
                room_count++;
            }
        }
    }

    Tunneler tunneler;
    tunneler.lifetime = 1000;
    tunneler.position = room_center(&mapgen->rooms[GetRandomValue(0, room_count)]);
    tunneler.direction = quadrant_disgusted_direction(tunneler.position);
    tunneler.width = 3;
    tunneler.padding = 1;
    tunneler.chance_to_turn = 10;
    dig(state, &tunneler);

    free(mapgen);
}

void generate_map(State *state) {
    for (int x = 0; x < GRID_WIDTH; x++) {
        for (int y = 0; y < GRID_HEIGHT; y++) {
            state->grid[x][y] = CELL_FLAG_WALL;
        }
    }

    {
        Cell center = { (GRID_WIDTH / 2), (GRID_HEIGHT / 2) };
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

    const int pivot_amount = 10;
    Cell pivots[pivot_amount];
    Cell random_range = {
        GRID_WIDTH / 4,
        GRID_HEIGHT / 4,
    };
    for (int i = 0; i < pivot_amount; i++) {
        pivots[i] = (Cell) {
            GetRandomValue(random_range.x, (GRID_WIDTH - 1) - random_range.x),
            GetRandomValue(random_range.y, (GRID_HEIGHT - 1) - random_range.y)
        };
    }

    int random_indices[pivot_amount];
    {
        int used_pivots_flags = 0;
        for (int i = 0; i < pivot_amount; i++) {
            int random_idx = GetRandomValue(0, pivot_amount - 1);
            int pivot_flag;

            do {
                random_idx = (random_idx + 1) % pivot_amount;
                pivot_flag = 1 << random_idx;
            } while (has_flag(used_pivots_flags, pivot_flag));

            used_pivots_flags |= pivot_flag;
            random_indices[i] = random_idx;
            printf("%i\n", random_idx);
        }
    }

    state->player.previous_position = pivots[random_indices[0]];
    state->player.position = pivots[random_indices[0]];

    for (int i = 1; i < pivot_amount + 1; i++) {
        int start_idx = random_indices[i % pivot_amount];
        Cell start = pivots[start_idx];

        dig_randomly(state, start, CELL_FLAG_ANY);

        int goal_idx = random_indices[(i + 1) % pivot_amount];
        Cell goal = pivots[goal_idx];

        dig_towards_target(state, start, goal, CELL_FLAG_ANY, 3);

        if (i < CREATURE_CAPACITY) {
            state->creatures[i].previous_position = pivots[start_idx];
            state->creatures[i].position = pivots[start_idx];
        }
    }

    MapGen *mapgen = (MapGen *)malloc(sizeof(MapGen));

    int room_count = 0;

    if (0) {
        for (int i = 0; i < ROOM_CAPACITY; i++) {
            Room room;
            Cell center = room_center(&room);
            if (try_add_random_room(state, &room)) {
                Cell closest_pivot = { INT32_MAX, INT32_MAX };
                for (int i = 0; i < pivot_amount; i++) {
                    if (manhattan_distance(center, pivots[i])) {
                        closest_pivot = pivots[i];
                    }
                }
                dig_towards_target(state, center, closest_pivot, CELL_FLAG_ANY, 1);
                room_count++;
            }
        }
    }

    free(mapgen);
}
