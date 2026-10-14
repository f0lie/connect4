/* External tests: the game source is included without modifying it. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef CONNECT4_SOURCE
#define CONNECT4_SOURCE "../connect4.c"
#endif
#define main connect4_program_main
#include CONNECT4_SOURCE
#undef main

static unsigned checks;

static void require(int condition, const char *description)
{
    ++checks;
    if (!condition) {
        fprintf(stderr, "FAIL: %s (check %u)\n", description, checks);
        exit(EXIT_FAILURE);
    }
}

static int in_board(int x, int y)
{
    return x >= 0 && x < SIZE_X && y >= 0 && y < SIZE_Y;
}

static void expect_win(char grid[SIZE_X][SIZE_Y], int x, int y,
                       char player, int win, const char *description)
{
    char before[SIZE_X][SIZE_Y];
    char chip = player;
    int result;
    memcpy(before, grid, sizeof before);
    result = checkWin(grid, x, y, &chip);
    if (result != !win || chip != (win ? player : (player == 'Y' ? 'R' : 'Y'))) {
        fprintf(stderr, "%s: last=(%d,%d), player=%c, expected win=%d, result=%d, chip=%c\n",
                description, x, y, player, win, result, chip);
        require(0, description);
    }
    require(result == !win, description);
    require(memcmp(before, grid, sizeof before) == 0, "checkWin preserves the board");
}

/* Independent oracle: enumerate complete four-cell segments containing last. */
static int reference_win(char grid[SIZE_X][SIZE_Y], int last_x, int last_y,
                         char chip)
{
    const int axes[4][2] = {{1, 0}, {0, 1}, {1, 1}, {1, -1}};
    int x, y, d, k;
    for (x = 0; x < SIZE_X; ++x) {
        for (y = 0; y < SIZE_Y; ++y) {
            for (d = 0; d < 4; ++d) {
                int same = 1, contains_last = 0;
                if (!in_board(x + 3 * axes[d][0], y + 3 * axes[d][1]))
                    continue;
                for (k = 0; k < 4; ++k) {
                    int xx = x + k * axes[d][0], yy = y + k * axes[d][1];
                    same = same && grid[xx][yy] == chip;
                    contains_last = contains_last || (xx == last_x && yy == last_y);
                }
                if (same && contains_last)
                    return 1;
            }
        }
    }
    return 0;
}

static void test_board_operations(void)
{
    char grid[SIZE_X][SIZE_Y], before[SIZE_X][SIZE_Y];
    int x, y;
    require(SIZE_X == 9 && SIZE_Y == 9, "original 9 by 9 dimensions");
    memset(grid, '?', sizeof grid);
    clearGrid(grid);
    for (x = 0; x < SIZE_X; ++x)
        for (y = 0; y < SIZE_Y; ++y)
            require(grid[x][y] == '-', "clearGrid empties every cell");
    for (x = 0; x < SIZE_X; ++x) {
        for (y = SIZE_Y - 1; y >= 0; --y) {
            char chip = (y % 2) ? 'R' : 'Y';
            require(placeChip(grid, x, chip) == y, "gravity returns exact row");
            require(grid[x][y] == chip, "placement uses selected chip");
        }
        memcpy(before, grid, sizeof before);
        require(placeChip(grid, x, 'R') == -1, "full column rejects a chip");
        require(memcmp(before, grid, sizeof before) == 0, "full column leaves board intact");
    }
}

static void test_all_straight_runs(void)
{
    const int axes[4][2] = {{1, 0}, {0, 1}, {1, 1}, {1, -1}};
    const char players[2] = {'Y', 'R'};
    char grid[SIZE_X][SIZE_Y];
    int length, d, x, y, p, k, last;
    /* Every legal location, axis, endpoint/interior, and both colors. This
       includes all corners and edges, and both 1+2 and 2+1 joined wins. */
    for (length = 1; length <= 5; ++length) {
        for (d = 0; d < 4; ++d) {
            for (x = 0; x < SIZE_X; ++x) {
                for (y = 0; y < SIZE_Y; ++y) {
                    if (!in_board(x + (length - 1) * axes[d][0],
                                  y + (length - 1) * axes[d][1]))
                        continue;
                    for (p = 0; p < 2; ++p) {
                        clearGrid(grid);
                        for (k = 0; k < length; ++k)
                            grid[x + k * axes[d][0]][y + k * axes[d][1]] = players[p];
                        for (last = 0; last < length; ++last)
                            expect_win(grid, x + last * axes[d][0], y + last * axes[d][1],
                                       players[p], length >= 4, "straight run");
                    }
                }
            }
        }
    }
}

static void test_separated_runs(void)
{
    const int axes[4][2] = {{1, 0}, {0, 1}, {1, 1}, {1, -1}};
    const char players[2] = {'Y', 'R'};
    char grid[SIZE_X][SIZE_Y];
    int d, x, y, p, k, gap;
    for (d = 0; d < 4; ++d) {
        for (x = 0; x < SIZE_X; ++x) {
            for (y = 0; y < SIZE_Y; ++y) {
                if (!in_board(x + 4 * axes[d][0], y + 4 * axes[d][1]))
                    continue;
                for (p = 0; p < 2; ++p) {
                    for (gap = 0; gap < 2; ++gap) {
                        clearGrid(grid);
                        for (k = 0; k < 5; ++k)
                            grid[x + k * axes[d][0]][y + k * axes[d][1]] =
                                k == 2 ? (gap ? '-' : players[1 - p]) : players[p];
                        for (k = 0; k < 5; ++k)
                            if (k != 2)
                                expect_win(grid, x + k * axes[d][0], y + k * axes[d][1],
                                           players[p], 0, "opponent or gap breaks a run");
                    }
                }
            }
        }
    }
}

static unsigned random_state = 0x31415926u;

static unsigned next_random(void)
{
    random_state = random_state * 1664525u + 1013904223u;
    return random_state;
}

static void test_no_false_wins_and_random_boards(void)
{
    char grid[SIZE_X][SIZE_Y];
    const char cells[3] = {'-', 'Y', 'R'};
    int x, y, test;
    clearGrid(grid);
    for (x = 3; x <= 5; ++x)
        for (y = 3; y <= 5; ++y)
            grid[x][y] = 'Y';
    expect_win(grid, 4, 4, 'Y', 0, "eight matching neighbors are only three per axis");

    clearGrid(grid);
    grid[0][8] = grid[1][8] = grid[2][8] = grid[8][0] = 'Y';
    expect_win(grid, 2, 8, 'Y', 0, "edge triples must not wrap around memory");
    clearGrid(grid);
    for (x = 0; x < 4; ++x)
        grid[x][8] = 'Y';
    grid[8][0] = 'Y';
    expect_win(grid, 8, 0, 'Y', 0, "unrelated existing line is not a last-move win");

    for (test = 0; test < 3000; ++test) {
        int last_x, last_y;
        char chip;
        for (x = 0; x < SIZE_X; ++x)
            for (y = 0; y < SIZE_Y; ++y)
                grid[x][y] = cells[(next_random() >> 16) % 3];
        last_x = (int)((next_random() >> 16) % SIZE_X);
        last_y = (int)((next_random() >> 16) % SIZE_Y);
        chip = (next_random() & 1u) ? 'Y' : 'R';
        grid[last_x][last_y] = chip;
        expect_win(grid, last_x, last_y, chip,
                   reference_win(grid, last_x, last_y, chip), "deterministic random board oracle");
    }
}

int main(void)
{
    test_board_operations();
    test_all_straight_runs();
    test_separated_runs();
    test_no_false_wins_and_random_boards();
    printf("PASS: %u board and win checks\n", checks);
    return 0;
}
