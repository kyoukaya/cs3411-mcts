#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/time.h>

#include "mcts.h"
#include "game.h"
#include "agent.h"

#define TRUE 1
#define FALSE 0

static int isGameWon(uint32_t board, uint32_t p);
void whiteBoxTests(void);

double ucb_const;

static void freeTree(Node *node) {
    for (int i = 0; i < SUBBOARD_SIZE && node->children[i] != NULL; i++) {
        freeTree(node->children[i]);
    }
    free(node);
}

static Node *mostVisitedChild(Node *node) {
    Node *highestNode = NULL;
    uint32_t highestVisited = 0;
    uint32_t i;

    for (i = 0; node->children[i] != NULL && i < SUBBOARD_SIZE; i++) {
        if (node->children[i]->visits > highestVisited) {
            highestVisited = node->children[i]->visits;
            highestNode = node->children[i];
        }
    }

    return highestNode;
}

int run_mcts(State *rootState, Move lastMove, uint32_t maxMs, double *confidence) {
    uint32_t i;
    Node *root = newNode(rootState, lastMove, NULL);
    State *state = calloc(1, sizeof(State));
    struct timeval start, curtime;
    gettimeofday(&start, NULL);

    for (i = 1; i < MAXITER; i++) {
        // Do a time check every 25000 iterations
        if ((i % 25000) == 0) {            
            gettimeofday(&curtime, NULL);
            uint32_t curMs = (curtime.tv_sec - start.tv_sec) * 1000 + (curtime.tv_usec - start.tv_usec) / 1000;
            if (curMs > maxMs) {
                break;
            }
        }
        Node *node = root;
        // Restore original state on each iteration.
        memcpy(state, rootState, sizeof(State));

        // Select
        while (node->nUntriedMoves == 0 && node->children[0] != NULL) {
            node = nodeSelectChild(node);
            stateDoMove(state, node->move);
        }

        // Expand
        if (state->gameStatus == GAME_NOT_TERMINAL) {
            Move move = node->untriedMoves[rand() % node->nUntriedMoves];
            stateDoMove(state, move);
            node = nodeAddChild(node, move, state);
        }

        // Playout
        statePlayout(state);

        // Backpropagate
        double winState[3];
        winState[state->playerLastMoved] = state->gameStatus;
        // Optimisation based on the assumption that it's a zero-sum game.
        winState[3 - state->playerLastMoved] = 1 - state->gameStatus;
        while (node != NULL) {
            nodeUpdate(node, winState[node->playerLastMoved]);
            node = node->parent;
        }
    }
    free(state);
    // return the move that was most visited.
    Node *highestNode = mostVisitedChild(root);
    double conf = highestNode->wins / highestNode->visits;
    if (verbose) {
        gettimeofday(&curtime, NULL);
        uint32_t curMs = (curtime.tv_sec - start.tv_sec) * 1000 + (curtime.tv_usec - start.tv_usec) / 1000;
        fprintf(stderr, "[%u]T:%d ", curMs, moveNo);
        for (int n = 0; n < SUBBOARD_SIZE && root->children[n] != NULL; n++) {
            fprintf(stderr, "%.2lf ",root->children[n]->wins / root->children[n]->visits);
        }
        fprintf(stderr, "\n");
        fprintf(stderr, "Mv: %d W/V: %.0lf/%u(%.2lf) iters: %d\n", highestNode->move,
               highestNode->wins, highestNode->visits,
               conf, i);
    }
    *confidence = conf;
    int ourMove = highestNode->move;
    freeTree(root);
    return ourMove;
}

State *initState(int board, int prev_move, int first_move) {
    State *newState = calloc(1, sizeof(State));
    // We shouldn't need to init state if the game is already terminal...
    newState->gameStatus = GAME_NOT_TERMINAL;
    newState->subBoard = prev_move;
    if (first_move == -1) {
        newState->me = CIRCLE_PLAYER;
        newState->board[board] = CROSS_PLAYER_START << prev_move;
        newState->playerLastMoved = CROSS_PLAYER;
    } else {
        newState->me = CROSS_PLAYER;
        newState->board[board] = CROSS_PLAYER_START << first_move;
        newState->board[first_move] |= CIRCLE_PLAYER_START << prev_move;
        newState->playerLastMoved = CIRCLE_PLAYER;
    }
    newState->opponent = 3 - newState->me;
    return newState;
}

void stateDoMove(State *state, Move move) {
    uint32_t moveMaker = 3 - state->playerLastMoved;
    int prevBoard = state->subBoard;
    state->board[state->subBoard] |= moveMaker == CIRCLE_PLAYER
                                         ? CIRCLE_PLAYER_START << move
                                         : CROSS_PLAYER_START << move;
    state->subBoard = move;
    state->playerLastMoved = moveMaker;
    state->gameStatus = stateResult(state, moveMaker, prevBoard);
}

void stateGetMoves(State *state, Move moves[SUBBOARD_SIZE],
                   uint32_t *numMoves) {
    uint32_t subBoard = state->board[state->subBoard];
    uint32_t mask = CIRCLE_PLAYER_START + CROSS_PLAYER_START;
    int n = 0;
    for (int i = 0; i < SUBBOARD_SIZE; i++) {
        // If Circle or Cross doesn't have a move in that square...
        if (!(subBoard & mask)) {
            moves[n] = i;
            ++n;
        }
        mask = mask << 1;
    }
    *numMoves = n;
}

void statePlayout(State *state) {
    uint32_t nMoves;
    Move moves[SUBBOARD_SIZE];
    while (state->gameStatus == GAME_NOT_TERMINAL) {
        stateGetMoves(state, moves, &nMoves);
        stateDoMove(state, moves[rand() % nMoves]);
    }
}

// EVIL BIT LEVEL OPTIMIZATION.
static int isBoardFull(uint32_t board) {
    uint32_t circles = board & ALL_CIRCLES_MASK;
    uint32_t crosses = (board & ALL_CROSSES_MASK) >> 9;
    return (circles | crosses) == ALL_CIRCLES_MASK;
}

// EVIL BIT LEVEL OPTIMIZATION BUT WORSE.
static int isGameWon(uint32_t board, uint32_t p) {
    --p;
    board = board >> (9 * p);
    return ((board & ROW0) == ROW0 || (board & ROW1) == ROW1 ||
            (board & ROW2) == ROW2 || (board & COL0) == COL0 ||
            (board & COL1) == COL1 || (board & COL2) == COL2 ||
            (board & DIA0) == DIA0 || (board & DIA1) == DIA1);
}

double stateResult(State *state, int player, int prevBoard) {
    uint32_t subBoard = state->board[prevBoard];

    if (isBoardFull(subBoard)) {
        return GAME_DRAWN;
    } else if (isGameWon(subBoard, player)) {
        return GAME_WON;
    } else if (isGameWon(subBoard, 3 - player)) {
        return GAME_LOST;
    }
    // Non terminal state.
    return GAME_NOT_TERMINAL;
}

Node *newNode(State *state, Move move, Node *parent) {
    Node *node = calloc(1, sizeof(Node));
    node->playerLastMoved = state->playerLastMoved;
    node->move = move;
    node->parent = parent;
    node->wins = 0.0;
    node->visits = 0;

    stateGetMoves(state, node->untriedMoves, &node->nUntriedMoves);
    return node;
}

Node *nodeSelectChild(Node *node) {
    Node *bestChild = NULL;
    double curUCT;
    double bestUCT = -INFINITY;
    double x = 0.25 * log((double)node->visits);
    // Profile has revealed this code to be extremely performance sensitive, but not
    // much can be optimized unfortunately!
    for (int i = 0; node->children[i] != NULL && i < SUBBOARD_SIZE; i++) {
        Node *curChild = node->children[i];
        curUCT = curChild->wins / (double)curChild->visits;
        curUCT += sqrt(x / (double)curChild->visits);
        if (curUCT > bestUCT) {
            bestUCT = curUCT;
            bestChild = curChild;
        }
    }

    return bestChild;
}

Node *nodeAddChild(Node *parent, Move move, State *state) {
    Node *childNode = newNode(state, move, parent);
    // Remove move from parent's untriedMoves
    uint32_t i;
    for (i = 0; i < parent->nUntriedMoves; i++) {
        if (parent->untriedMoves[i] == move) {
            // shift the rest of the array left.
            for (; i < (parent->nUntriedMoves - 1); i++) {
                parent->untriedMoves[i] = parent->untriedMoves[i + 1];
            }
            break;
        };
    }
    parent->nUntriedMoves--;
    // Add child into parent node.
    for (i = 0; i < SUBBOARD_SIZE; i++) {
        if (parent->children[i] == NULL) {
            parent->children[i] = childNode;
            break;
        }
    }
    return childNode;
}

void nodeUpdate(Node *node, double result) {
    node->visits++;
    node->wins += result;
}

void printBoard(State *state) {
    int board[10][10];
    int i, j;
    for (i = 0; i < SUBBOARD_SIZE; i++) {
        uint32_t c_mask = CIRCLE_PLAYER_START;
        uint32_t x_mask = CROSS_PLAYER_START;
        for (j = 0; j < SUBBOARD_SIZE; j++) {
            if (state->board[i] & c_mask) {
                board[i + 1][j + 1] = 1;
            } else if (state->board[i] & x_mask) {
                board[i + 1][j + 1] = 0;
            } else {
                board[i + 1][j + 1] = 2;
            }
            c_mask = c_mask << 1;
            x_mask = x_mask << 1;
        }
    }
    print_board(stdout, board, 0, 0);
}

void whiteBoxTests(void) {
    State *state = calloc(1, sizeof(State));
    // Testing
    state->board[0] = ROW0;
    printf("Testing ROW0\n");
    printBoard(state);
    assert(stateResult(state, CIRCLE_PLAYER, 0) == GAME_WON);
    assert(stateResult(state, CROSS_PLAYER, 0) == GAME_LOST);

    state->board[0] = ROW1;
    printf("Testing ROW1\n");
    printBoard(state);
    assert(stateResult(state, CIRCLE_PLAYER, 0) == GAME_WON);
    assert(stateResult(state, CROSS_PLAYER, 0) == GAME_LOST);

    state->board[0] = ROW2;
    printf("Testing ROW2\n");
    printBoard(state);
    assert(stateResult(state, CIRCLE_PLAYER, 0) == GAME_WON);
    assert(stateResult(state, CROSS_PLAYER, 0) == GAME_LOST);

    state->board[0] = COL0;
    printf("Testing COL0\n");
    printBoard(state);
    assert(stateResult(state, CIRCLE_PLAYER, 0) == GAME_WON);
    assert(stateResult(state, CROSS_PLAYER, 0) == GAME_LOST);

    state->board[0] = COL1;
    printf("Testing COL1\n");
    printBoard(state);
    assert(stateResult(state, CIRCLE_PLAYER, 0) == GAME_WON);
    assert(stateResult(state, CROSS_PLAYER, 0) == GAME_LOST);

    state->board[0] = COL2;
    printf("Testing COL2\n");
    printBoard(state);
    assert(stateResult(state, CIRCLE_PLAYER, 0) == GAME_WON);
    assert(stateResult(state, CROSS_PLAYER, 0) == GAME_LOST);

    state->board[0] = DIA0;
    printf("Testing DIA0\n");
    printBoard(state);
    assert(stateResult(state, CIRCLE_PLAYER, 0) == GAME_WON);
    assert(stateResult(state, CROSS_PLAYER, 0) == GAME_LOST);

    state->board[0] = DIA1;
    printf("Testing DIA1\n");
    printBoard(state);
    assert(stateResult(state, CIRCLE_PLAYER, 0) == GAME_WON);
    assert(stateResult(state, CROSS_PLAYER, 0) == GAME_LOST);
}
