#ifndef MAIN_H
#define MAIN_H

#include <stdint.h>
#include <math.h>
#include "../raylib/include/raylib.h"

#define GRID_WIDTH 128
#define GRID_HEIGHT 128
#define GAME_WIDTH 40
#define GAME_HEIGHT 30
#define GRID_LARGEST_SIDE ((GRID_WIDTH > GRID_HEIGHT) ? GRID_WIDTH : GRID_HEIGHT)
#define CELLAMOUNT (GRID_WIDTH * GRID_HEIGHT)
#define CELLSIZE 40
#define HALF_CELLSIZE (CELLSIZE / 2)
#define SCREEN_WIDTH (CELLSIZE * GRID_WIDTH)
#define SCREEN_HEIGHT (CELLSIZE * GRID_HEIGHT)
#define CREATURE_CAPACITY 2
#define TIME_PER_TURN 0.05f
#define TIME_PER_ANIMATION 0.4f
#define KEY_REPEAT_THRESHOLD 0.3f
#define NO_DIRECTION 255
#define INVALID_CELL ((IntX2) { -1, -1 })

#define COLOR_UNDISCOVERED ((Color){0,0,0,255})
#define COLOR_GROUND_VISIBLE ((Color){64,32,0,255})
#define COLOR_GROUND_INVISIBLE ((Color){0,0,0,255})
#define COLOR_WALL_VISIBLE ((Color){128,64,0,255})
#define COLOR_WALL_INVISIBLE ((Color){32,32,32,255})
#define COLOR_PLAYER_PATH ((Color){0,255,0,64})
#define COLOR_CREATURE_PLAYER ((Color){0,128,255,255})
#define COLOR_CREATURE_ENEMY ((Color){255,128,0,255})
#define COLOR_CREATURE_ENEMY2 ((Color){255,0,0,255})

typedef uint8_t uint8;
typedef uint16_t uint16;

enum StateFlags {
    GAME_FLAG_READY_FOR_UPDATE = 1 << 0,
    GAME_FLAG_IS_MOVING = 1 << 1,
};

enum Orthagonal {
    ORTHAGONAL_N,
    ORTHAGONAL_W,
    ORTHAGONAL_S,
    ORTHAGONAL_E,
};

enum Diagonal {
    DIAGONAL_NE,
    DIAGONAL_NW,
    DIAGONAL_SW,
    DIAGONAL_SE,
};

enum CellFlags {
    CELL_FLAG_ANY = 0,
    CELL_FLAG_DISCOVERED = 1 << 0,
    CELL_FLAG_VISIBLE = 1 << 1,
    CELL_FLAG_WALKABLE = 1 << 2,
    CELL_FLAG_WALL = 1 << 3,
    CELL_FLAG_CREATURE = 1 << 4,

    CELL_FLAG_PLAYER_WALKABLE = (CELL_FLAG_WALKABLE | CELL_FLAG_DISCOVERED),
    CELL_FLAG_CREATURE_WALKABLE = (CELL_FLAG_WALKABLE),
};

typedef struct IntX2 {
    int x;
    int y;
} IntX2;

bool intX2_eq(IntX2 a, IntX2 b) { return (a.x == b.x) && (a.y == b.y); }
bool intX2_neq(IntX2 a, IntX2 b) { return (a.x != b.x) || (a.y != b.y); }
IntX2 intX2_add(IntX2 a, IntX2 b) { return (IntX2) { (a.x + b.x), (a.y) + b.y }; }
IntX2 intX2_subtract(IntX2 a, IntX2 b) { return (IntX2) { (a.x - b.x), (a.y) - b.y }; }
IntX2 intX2_multiply(IntX2 a, int factor) { return (IntX2) { (a.x * factor), (a.y) * factor }; }
IntX2 intX2_divide(IntX2 a, int divisor) { return (IntX2) { (a.x / divisor), (a.y) / divisor }; }

typedef struct CoordAndDirection {
    IntX2 coord;
    uint8 direction;
} CoordAndDirection;

typedef struct ANode {
    IntX2 position;
    int g_cost;
    int f_cost;
    struct ANode *came_from;
} ANode;

typedef struct AStar {
    ANode all_list[GRID_WIDTH][GRID_HEIGHT];
    int open_list_count;
    ANode* open_list[CELLAMOUNT];
    int closed_list_count;
    ANode* closed_list[CELLAMOUNT];
} AStar;

typedef enum CreatureType {
    CREATURE_PLAYER,
    CREATURE_DIGGER,
    CREATURE_EVIL_TRIANGLE,
    CREATURE_BIG_EVIL_TRIANGLE,
} CreatureType;

enum CreatureFlags {
    CREATURE_FLAG_NONE,
    CREATURE_FLAG_DISCOVERED = 1 << 0,
    CREATURE_FLAG_VISIBLE = 1 << 1,
};

typedef struct Creature {
    CreatureType type;
    int flags;
    IntX2 previous_position;
    IntX2 position;
    uint16 direction;
    union {
        IntX2 last_known_player_location;
    };
} Creature;

typedef struct State {
    IntX2 game_offset;
    int flags;
    IntX2 mouse_current;
    IntX2 mouse_target;
    uint8 grid[GRID_WIDTH][GRID_HEIGHT];
    AStar a_star;
    Creature player;
    Creature creatures[CREATURE_CAPACITY];
    float game_timer;
    float turn_time;
    float animation_timer;
    float key_repeat_timer;
} State;

IntX2 screen_to_game_position(State *state, Vector2 screen_position) {
    IntX2 game_position = {
        .x = (screen_position.x / CELLSIZE) + state->game_offset.x,
        .y = (screen_position.y / CELLSIZE) + state->game_offset.y,
    };

    if (game_position.x < 0) {
        game_position.x = 0;
    } else if (game_position.x >= (GRID_WIDTH - 1)) {
        game_position.x = GRID_WIDTH - 1;
    }

    if (game_position.y < 0) {
        game_position.y = 0;
    } else if (game_position.y >= (GRID_HEIGHT - 1)) {
        game_position.y = GRID_HEIGHT - 1;
    }

    return game_position;
}

IntX2 get_cell_local_position(State *state, IntX2 local_position) {
    return (IntX2) {
        local_position.x - state->game_offset.x,
        local_position.y - state->game_offset.y,
    };
}

IntX2 astar_path(State *state, IntX2 start, IntX2 goal, int walkable_flags);

static inline bool has_flag(int flags, int flag) {
    return (flags & flag) == flag;
}

static inline int manhattan_distance(IntX2 a, IntX2 b) {
    return abs(a.x - b.x) + abs(a.y - b.y);
}

static inline bool is_cell_out_of_bounds(State *state, IntX2 cell) {
    return (
        cell.x < 0 || cell.x > (GRID_WIDTH - 1) ||
        cell.y < 0 || cell.y > (GRID_HEIGHT - 1)
    );
}

static inline bool is_cell_valid(State *state, IntX2 cell, int cell_flags) {
    return (
        !is_cell_out_of_bounds(state, cell) &&
        (state->grid[cell.x][cell.y] & cell_flags) == cell_flags
    );
}

IntX2 cell_in_direction(IntX2 position, uint8 direction) {
    switch (direction) {
    case ORTHAGONAL_N: return (IntX2) { position.x, position.y - 1 };
    case ORTHAGONAL_W: return (IntX2) { position.x - 1, position.y };
    case ORTHAGONAL_S: return (IntX2) { position.x, position.y + 1 };
    case ORTHAGONAL_E: return (IntX2) { position.x + 1, position.y };
    default: return position;
    }
}

IntX2 get_turn_offset(State *state, Creature *c) {
    return (IntX2) {
        (((c->previous_position.x - c->position.x)) * (CELLSIZE - state->turn_time)),
        (((c->previous_position.y - c->position.y)) * (CELLSIZE - state->turn_time)),
    };
}

IntX2 get_turn_position(State *state, Creature *c) {
    IntX2 player_turn_offset = get_turn_offset(state, &state->player);

    IntX2 current = get_cell_local_position(state, c->position);
    IntX2 previous = get_cell_local_position(state, c->previous_position);

    return (IntX2) {
        (previous.x * CELLSIZE) + ((current.x - previous.x) * state->turn_time) + HALF_CELLSIZE - player_turn_offset.x,
        (previous.y * CELLSIZE) + ((current.y - previous.y) * state->turn_time) + HALF_CELLSIZE - player_turn_offset.y,
    };
}

#endif