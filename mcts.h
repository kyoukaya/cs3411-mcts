#ifndef __MCTS_H__
#define __MCTS_H__

#include <stdint.h>
// Default value UCB exploration param. sqrt(2)
#define UCB_CONST 1.41421
// sqrt(0.4) seems to be the best for Circle games with a ~20% WR
// As circle, we are most likely to win if the first move board 1/2 (symmetry applies)
// and least likely to win if the first move is on board 5.
#define UCB_CIRCLE12 0.632455
#define UCB_CIRCLE5 1.09545

// Amount of iterations use to run the first turn, mostly used for benchmarking as
// it's highly unlikely for a good strategy to emerge in the early game.
#define INITIAL_ITER 1000000
// In the late game, we cap the iterations so we don't spin for too long as the game is
// pretty much decided at this point.
#define MAXITER 2000000
// In seconds
#define MAX_TARGET_TURN_TIME 4
#define FAST_TARGET_TURN_TIME 2

#define EMPTY_SQUARE 0
#define CIRCLE_PLAYER 1
#define CROSS_PLAYER 2

#define SUBBOARD_SIZE 9

#define GAME_NOT_TERMINAL -1.0
#define GAME_LOST 0
#define GAME_WON 1
#define GAME_DRAWN 0.5

// Scary bit constants below
#define CROSS_PLAYER_START 512
#define CIRCLE_PLAYER_START 1
#define ALL_CIRCLES_MASK 511
// 511 << 9
#define ALL_CROSSES_MASK 261632
// wtf
#define ROW0 7
#define ROW1 56
#define ROW2 448
#define COL0 73
#define COL1 146
#define COL2 292
#define DIA0 273
#define DIA1 84

// Unsigned char
typedef uint8_t Move;

// Game state.
typedef struct state {
    // GAME_NOT_TERMINAL/LOST/WON/DRAWN
    double gameStatus;
    // CIRCLE_PLAYER/CROSS_PLAYER
    int opponent;
    // CIRCLE_PLAYER/CROSS_PLAYER
    int me;
    int playerLastMoved;
    // Which sub-board the game is currently on.
    int subBoard;
    /* Each subboard is divided into 2 9 bit sections
     * Starting with the least 9 bits for Circle and then the nex 9 for Cross. */
    uint32_t board[SUBBOARD_SIZE];
} State;

typedef struct _mctsNode {
    // Root node has NULL for its parent.
    struct _mctsNode *parent;
    // The move that got us to this node.
    Move move;
    int playerLastMoved;
    struct _mctsNode *children[SUBBOARD_SIZE];
    /* Not NULL terminated, iterate with nUntriedMoves */
    Move untriedMoves[SUBBOARD_SIZE];
    // Required to pick a random move.
    uint32_t nUntriedMoves;
    double wins;
    uint32_t visits;
} Node;

int run_mcts(State *state, Move lastMove, uint32_t maxIter);

Node *newNode(State *state, Move move, Node *parent);
// Use the UCB1 formula to select a child node. Often a constant UCTK is applied
// to vary the amount of exploration versus exploitation.
Node *nodeSelectChild(Node *node);
// Remove m from untriedMoves and add a new child node for this move. Return the
// added child node
Node *nodeAddChild(Node *node, Move move, State *state);
// Update this node - one additional visit and result additional wins. result
// must be from the viewpoint of playerJustmoved.
void nodeUpdate(Node *node, double result);

State *initState(int board, int prev_move, int first_move);
void stateDoMove(State *state, Move move);
void stateGetMoves(State *state, Move moves[SUBBOARD_SIZE], uint32_t *numMoves);
void statePlayout(State *state);
double stateResult(State *state, int player, int prevBoard);

void printNode(Node *node);
// Hacky adapter to provided print_board function.
void printBoard(State *state);

#endif
