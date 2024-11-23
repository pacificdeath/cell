#ifndef MAIN_H
#define MAIN_H

#include <stdint.h>
#include <math.h>
#include "../raylib/include/raylib.h"

#define GRID_WIDTH 40
#define GRID_HEIGHT 30
#define GRID_LARGEST_SIDE ((GRID_WIDTH > GRID_HEIGHT) ? GRID_WIDTH : GRID_HEIGHT)
#define CELLAMOUNT (GRID_WIDTH * GRID_HEIGHT)
#define CELLSIZE 40
#define SCREEN_WIDTH (CELLSIZE * GRID_WIDTH)
#define SCREEN_HEIGHT (CELLSIZE * GRID_HEIGHT)
#define CREATURE_CAPACITY 2
#define TIME_PER_TURN 0.05f
#define TIME_PER_ANIMATION 0.4f
#define NO_DIRECTION 255
#define INVALID_CELL ((Cell){ -1, -1 })

#define COLOR_UNDISCOVERED ((Color){0,0,0,255})
#define COLOR_GROUND_VISIBLE ((Color){64,32,0,255})
#define COLOR_GROUND_INVISIBLE ((Color){32,32,32,255})
#define COLOR_WALL_VISIBLE ((Color){128,64,0,255})
#define COLOR_WALL_INVISIBLE ((Color){64,64,64,255})
#define COLOR_PLAYER_PATH ((Color){0,255,0,64})
#define COLOR_CREATURE_PLAYER ((Color){0,128,255,255})
#define COLOR_CREATURE_ENEMY ((Color){255,128,0,255})
#define COLOR_CREATURE_ENEMY2 ((Color){255,0,0,255})

typedef uint8_t uint8;
typedef uint16_t uint16;

typedef enum Orthagonal {
    ORTHAGONAL_N,
    ORTHAGONAL_W,
    ORTHAGONAL_S,
    ORTHAGONAL_E,
} Orthagonal;

typedef enum Diagonal {
    DIAGONAL_NE,
    DIAGONAL_NW,
    DIAGONAL_SW,
    DIAGONAL_SE,
} Diagonal;

typedef enum CellBits {
    CELL_BIT_ANY = 0,
    CELL_BIT_DISCOVERED = 1 << 0,
    CELL_BIT_VISIBLE = 1 << 1,
    CELL_BIT_WALKABLE = 1 << 2,
    CELL_BIT_WALL = 1 << 3,
    CELL_BIT_CREATURE = 1 << 4,

    PLAYER_WALKABLE = (CELL_BIT_WALKABLE | CELL_BIT_DISCOVERED),
    CREATURE_WALKABLE = (CELL_BIT_WALKABLE),
} CellBits;

typedef struct Cell {
    int x;
    int y;
} Cell;

bool cell_eq(Cell a, Cell b) {
    return (a.x == b.x) && (a.y == b.y);
}

bool cell_neq(Cell a, Cell b) {
    return (a.x != b.x) || (a.y != b.y);
}

typedef struct CoordAndDirection {
    Cell coord;
    uint8 direction;
} CoordAndDirection;

typedef struct ANode {
    Cell position;
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

typedef enum CreatureBits {
    CREATURE_BIT_NONE,
    CREATURE_BIT_DISCOVERED = 1 << 0,
    CREATURE_BIT_VISIBLE = 1 << 1,
} CreatureBits;

typedef struct Creature {
    CreatureType type;
    int bits;
    Cell previous_position;
    Cell position;
    uint16 direction;
    union {
        Cell last_known_player_location;
    };
    
} Creature;

typedef struct State {
    Cell mouse_target_coord;
    uint8 grid[GRID_WIDTH][GRID_HEIGHT];
    AStar a_star;
    Creature player;
    Creature creatures[CREATURE_CAPACITY];
    float game_timer;
    float animation_timer;
} State;

bool has_bit(int flags, int flag) {
    return (flags & flag) == flag;
}

int manhattan_distance(Cell a, Cell b) {
    return abs(a.x - b.x) + abs(a.y - b.y);
}

bool is_valid_cell(State *state, Cell position, int cell_bits) {
    return (
        position.x >= 0 && position.x < GRID_WIDTH &&
        position.y >= 0 && position.y < GRID_HEIGHT &&
        (state->grid[position.x][position.y] & cell_bits) == cell_bits
    );
}

Cell cell_in_direction(Cell position, uint8 direction) {
    switch (direction) {
    case ORTHAGONAL_N: return (Cell) { position.x, position.y - 1 };
    case ORTHAGONAL_W: return (Cell) { position.x - 1, position.y };
    case ORTHAGONAL_S: return (Cell) { position.x, position.y + 1 };
    case ORTHAGONAL_E: return (Cell) { position.x + 1, position.y };
    default: return position;
    }
}

#endif