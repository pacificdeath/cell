#include <math.h>
#include "main.h"

Cell astar_path(State *state, Cell start, Cell goal, int walkable_flags) {
    if (cell_eq(start, goal)) {
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
    a->open_list[0]->f_cost = manhattan_distance(start, goal);

    a->closed_list_count = 0;

    while (a->open_list_count > 0) {
        int current_idx = 0;
        for (int i = 1; i < a->open_list_count; i++) {
            int other_cost = a->open_list[i]->f_cost;
            int current_cost = a->open_list[current_idx]->f_cost;
            if (other_cost < current_cost) {
                current_idx = i;
            }
        }
        ANode *current = a->open_list[current_idx];
        if (current->position.x == goal.x && current->position.y == goal.y) {
            while (cell_neq(current->came_from->position, start)) {
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

        const int neighbour_count = 4;
        Cell neighbours[neighbour_count];
        neighbours[0] = (Cell) { .x = current->position.x,     .y = current->position.y + 1    };
        neighbours[1] = (Cell) { .x = current->position.x - 1, .y = current->position.y        };
        neighbours[2] = (Cell) { .x = current->position.x,     .y = current->position.y - 1    };
        neighbours[3] = (Cell) { .x = current->position.x + 1, .y = current->position.y        };

        for (int i = 0; i < neighbour_count; i++) {
            if (!is_cell_valid(state, neighbours[i], walkable_flags)) {
                continue;
            }
            ANode *n = &(a->all_list[neighbours[i].x][neighbours[i].y]);
            int tentative_cost = current->g_cost + 1;
            if (tentative_cost < n->g_cost) {
                n->came_from = current;
                n->g_cost = tentative_cost;
                n->f_cost = n->g_cost + manhattan_distance(n->position, goal);
                bool neighbour_in_closed_list = false;
                for (int i = 0; i < a->closed_list_count; i++) {
                    if (cell_eq(a->closed_list[i]->position, n->position)) {
                        neighbour_in_closed_list = true;
                        break;
                    }
                }
                if (neighbour_in_closed_list) {
                    continue;
                }
                bool neighbour_in_open_list = false;
                for (int i = 0; i < a->open_list_count; i++) {
                    if (cell_eq(a->open_list[i]->position, n->position)) {
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

CoordAndDirection bounce_path(State *state, Cell start, uint8 direction) {
    Cell n = {start.x, start.y - 1};
    Cell w = {start.x - 1, start.y};
    Cell s = {start.x, start.y + 1};
    Cell e = {start.x + 1, start.y};

    bool n_wall = !is_cell_valid(state, n, CELL_FLAG_WALKABLE);
    bool w_wall = !is_cell_valid(state, w, CELL_FLAG_WALKABLE);
    bool s_wall = !is_cell_valid(state, s, CELL_FLAG_WALKABLE);
    bool e_wall = !is_cell_valid(state, e, CELL_FLAG_WALKABLE);

    Cell ne = {start.x + 1, start.y - 1};
    Cell nw = {start.x - 1, start.y - 1};
    Cell sw = {start.x - 1, start.y + 1};
    Cell se = {start.x + 1, start.y + 1};

    bool ne_open = is_cell_valid(state, ne, CELL_FLAG_WALKABLE);
    bool nw_open = is_cell_valid(state, nw, CELL_FLAG_WALKABLE);
    bool sw_open = is_cell_valid(state, sw, CELL_FLAG_WALKABLE);
    bool se_open = is_cell_valid(state, se, CELL_FLAG_WALKABLE);

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
            return (CoordAndDirection) { start, 0 };
    }

    switch (bounce) {
        case DIAGONAL_NE: return (CoordAndDirection){ne, DIAGONAL_NE};
        case DIAGONAL_NW: return (CoordAndDirection){nw, DIAGONAL_NW};
        case DIAGONAL_SE: return (CoordAndDirection){se, DIAGONAL_SE};
        case DIAGONAL_SW: return (CoordAndDirection){sw, DIAGONAL_SW};
    }

    return (CoordAndDirection) { start, direction };
}

Cell random_wander(State *state, Cell start, uint8 direction) {
    int random = GetRandomValue(0, 3);
    Cell backtrack_cell = get_cell_in_direction(start, (direction + 2) % 4, 1);
    for (int i = 0; i < 4; i++) {
        Cell position;
        switch (random) {
        case 0: position = (Cell) {start.x, start.y - 1}; break;
        case 1: position = (Cell) {start.x - 1, start.y}; break;
        case 2: position = (Cell) {start.x, start.y + 1}; break;
        case 3: position = (Cell) {start.x + 1, start.y}; break;
        }
        bool valid = is_cell_valid(state, position, CELL_FLAG_WALKABLE);
        bool backtrack = cell_eq(backtrack_cell, position);
        if (valid && !backtrack) {
            return position;
        }
        random = (random + 1) % 4;
    }
    bool okay_backtrack_if_that_is_the_only_way = is_cell_valid(state, backtrack_cell, CELL_FLAG_WALKABLE);
    if (okay_backtrack_if_that_is_the_only_way) {
        return backtrack_cell;
    }
    return start;
}
