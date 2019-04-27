#ifndef __MCTS_H__
#define __MCTS_H__

#include <stdint.h>

#define INITIAL_ITER 100000
#define MAXITER 2000000
// In seconds
#define TARGET_TURN_TIME 3.5

#define EMPTY_SQUARE 0
#define CIRCLE_PLAYER 1
#define CROSS_PLAYER 2

#define SUBBOARD_SIZE 9

#define GAME_NOT_TERMINAL -1.0
#define GAME_LOST 0
#define GAME_WON 1
#define GAME_DRAWN 0.5

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

typedef uint8_t Move;
// Local boards are accessed by board[0..8]
// [0..8] to bit is 1 << num
// Board state.
typedef struct state {
    // GAME_NOT_TERMINAL/LOST/WON/DRAWN
    double gameStatus;
    int opponent;
    int me;
    // int lastBoard; // TODO stateResult needs to check a board
    int playerLastMoved;
    // Which sub-board the game is currently on.
    int subBoard;
    // 0 = empty, 1 = X, 2 = 0
    uint32_t board[SUBBOARD_SIZE];
} State;

typedef struct _mctsNode {
    // The move that got us to this node, NULL for the root node.
    int playerLastMoved;
    Move move;
    struct _mctsNode *parent;

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
// so we have lambda c: c.wins/c.visits + UCTK *
// sqrt(2*log(self.visits)/c.visits to vary the amount of exploration versus
// exploitation.
Node *nodeSelectChild(Node *node);
// Remove m from untriedMoves and add a new child node for this move. Return the
// added child node
Node *nodeAddChild(Node *node, Move move, State *state);
// Update this node - one additional visit and result additional wins. result
// must be from the viewpoint of playerJustmoved.
void nodeUpdate(Node *node, double result);

State *initState(int board, int prev_move, int first_move);
// State deep clone.
void stateDoMove(State *state, Move move);
void stateGetMoves(State *state, Move moves[SUBBOARD_SIZE], uint32_t *numMoves);
void statePlayout(State *state);
double stateResult(State *state, int player, int prevBoard);

void printNode(Node *node);
// Hacky adapter to provided print_board function.
void printBoard(State *state);

#endif
