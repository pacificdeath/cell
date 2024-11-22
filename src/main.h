#ifndef MAIN_H
#define MAIN_H

#include <stdint.h>
#include "../raylib/include/raylib.h"

#define GRID_WIDTH 40
#define GRID_HEIGHT 30
#define CELLAMOUNT (GRID_WIDTH * GRID_HEIGHT)
#define CELLSIZE 40
#define SCREEN_WIDTH (CELLSIZE * GRID_WIDTH)
#define SCREEN_HEIGHT (CELLSIZE * GRID_HEIGHT)
#define VISIBILITY_SPHERE_CELL_AMOUNT 40
#define CREATURE_CAPACITY 2
#define TIME_PER_TURN 0.05f
#define TIME_PER_ANIMATION 0.2f

#define COLOR_UNDISCOVERED ((Color){0,0,0,255})
#define COLOR_EMPTY ((Color){64,32,0,255})
#define COLOR_WALL ((Color){128,64,0,255})
#define COLOR_PLAYER_PATH ((Color){0,255,0,64})
#define COLOR_CREATURE_PLAYER ((Color){0,128,255,255})
#define COLOR_CREATURE_ENEMY ((Color){255,128,0,255})
#define COLOR_CREATURE_ENEMY2 ((Color){255,0,0,255})

typedef uint8_t uint8;

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

enum CellBits {
    CELL_BIT_ANY = 0,
    CELL_BIT_UNDISCOVERED = 1 << 0,
    CELL_BIT_VISIBLE = 1 << 1,
    CELL_BIT_WALKABLE = 1 << 2,
    CELL_BIT_WALL = 1 << 3,
    CELL_BIT_CREATURE = 1 << 4,
};

struct Coord {
    int x;
    int y;

    bool operator==(const Coord c) {
        return (x == c.x) && (y == c.y);
    }

    bool operator!=(const Coord c) {
        return (x != c.x) || (y != c.y);
    }
};

struct CoordAndDirection {
    Coord coord;
    uint8 direction;
};

struct ANode {
    Coord position;
    int g_cost;
    int f_cost;
    ANode *came_from;
};

struct AStar {
    ANode all_list[GRID_WIDTH][GRID_HEIGHT];
    int open_list_count;
    ANode* open_list[CELLAMOUNT];
    int closed_list_count;
    ANode* closed_list[CELLAMOUNT];
};

enum CreatureType {
    CREATURE_PLAYER,
    CREATURE_DIGGER,
    CREATURE_EVIL_TRIANGLE,
    CREATURE_BIG_EVIL_TRIANGLE,
};

enum CreatureBits {
    CREATURE_BIT_NONE,
    CREATURE_BIT_VISIBLE,
};

struct Creature {
    CreatureType type;
    int bits;
    Coord previous_position;
    Coord position;
    uint8 direction;
};

struct State {
    Coord visibility_sphere[VISIBILITY_SPHERE_CELL_AMOUNT];
    Coord mouse_target_coord;
    uint8 grid[GRID_WIDTH][GRID_HEIGHT];
    AStar a_star;
    Creature player;
    Creature creatures[CREATURE_CAPACITY];
    float game_timer;
    float animation_timer;
};

#endif