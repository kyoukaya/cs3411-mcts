#ifndef __MCTS_H__
#define __MCTS_H__

#include <stdint.h>

// In the late game, we cap the iterations so we don't spin for too long as the
// game is pretty much decided at this point.
#define MAXITER 100000

// Time controls in ms.
// Maximum turn time, used in the mid-game.
#define MAX_TARGET_TURN_TIME 4200
// Early game time controls.
#define FIRST_TURN_TIME 1000
#define FAST_TARGET_TURN_TIME 2000
// Victory or defeat should be obvious at this point.
#define END_GAME_TURN_TIME 1750

#define EMPTY_SQUARE 0
#define CIRCLE_PLAYER 1
#define CROSS_PLAYER 2
#define BOARD_SIZE 9
#define GAME_NOT_TERMINAL -1.0
#define GAME_LOST 0.0
#define GAME_WON 1.0
#define GAME_DRAWN 0.5

// Scary bit constants below
#define CROSS_PLAYER_START 512u
#define CIRCLE_PLAYER_START 1u
#define ALL_CIRCLES_MASK 511u
// 511 << 9
#define ALL_CROSSES_MASK 261632u
// wtf
#define ROW0 7u
#define ROW1 56u
#define ROW2 448u
#define COL0 73u
#define COL1 146u
#define COL2 292u
#define DIA0 273u
#define DIA1 84u

// Unsigned char
typedef uint8_t Move;

// Game state.
typedef struct state {
    // GAME_NOT_TERMINAL/LOST/WON/DRAWN
    double gameStatus;
    // CIRCLE_PLAYER/CROSS_PLAYER
    int opponent;
    int me;
    int playerLastMoved;
    // Which sub-board the game is currently on.
    int subBoard;
    /* Each subboard is divided into 2 9 bit sections. Starting with the least 9
     * bits for Circle and then the nex 9 for Cross. */
    uint32_t board[BOARD_SIZE];
} State;

typedef struct _mctsNode {
    // Root node has NULL for its parent.
    struct _mctsNode *parent;
    // The move that got us to this node.
    Move move;
    int playerLastMoved;
    struct _mctsNode *children[BOARD_SIZE];
    Move untriedMoves[BOARD_SIZE];
    // Required to pick a random move.
    uint32_t nUntriedMoves;
    double wins;
    uint32_t visits;
} Node;

// Returns move [0..8]
int run_mcts(State *rootState, Move lastMove, uint32_t maxMs);

State *initState(int board, int prev_move, int first_move);
void stateDoMove(State *state, Move move);

// Hacky adapter to provided print_board function.
void printBoard(State *state);
void whiteBoxTests(void);

#endif
