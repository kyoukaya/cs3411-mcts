/*********************************************************
 *  agent.c
 *  Nine-Board Tic-Tac-Toe Agent
 *  COMP3411/9414/9814 Artificial Intelligence
 *  Alan Blair, CSE, UNSW
 */

/* Player code written by Bryan Chew (Z5180123)
 * Uses a Monte Carlo Tree Search to select its next move.
 * 
 * I'd decided to use a MCTS based approach to game playing as I'd tried but failed in
 * COMP1511 and wanted another go at it, I was also not confident in my domain knowledge
 * of this version of Tic Tac Toe and didn't think I'd be able to come up with a decent
 * position evaluator.
 * 
 * Naturally since MCTS is incredibly computationally reliant, I'd decided to write it
 * in C to best optimize the code I can. Additionally using gprof to profile and optimize
 * the hot code paths, namely; isBoardFull, isGameWon.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "mcts.h"
#include "common.h"
#include "agent.h"
#include "game.h"

#define MAX_MOVE 81

State *state;
int moveNo = 0;
int benchmarkedIters = 0;
// Time we spent thinking
uint32_t totalMs = 0;
// Boards/squares Indexed from 1.
int firstMove[2];
int verbose = FALSE;
int targetTurnTime = FAST_TARGET_TURN_TIME;

/*********************************************************/ /*
    Print usage information and exit
 */
void usage(char argv0[]) {
    printf("Usage: %s\n", argv0);
    printf("       -v");
    printf("       [-p port]\n");  // tcp port
    printf("       [-h host]\n");  // tcp host
    // printf("       [-c UCB const]\n");
    printf("       [-t time(s)]\n");
    exit(1);
}

/*********************************************************/ /*
    Parse command-line arguments
 */
void agent_parse_args(int argc, char *argv[]) {
    int i = 1;
    while (i < argc) {
        if (strcmp(argv[i], "-p") == 0) {
            if (i + 1 >= argc) {
                usage(argv[0]);
            }
            port = atoi(argv[i + 1]);
            i += 2;
        } else if (strcmp(argv[i], "-h") == 0) {
            if (i + 1 >= argc) {
                usage(argv[0]);
            }
            host = argv[i + 1];
            i += 2;
        }
        // else if (strcmp(argv[i], "-c") == 0) {
        //     if (i + 1 >= argc) {
        //         usage(argv[0]);
        //     }
        //     if (!sscanf(argv[i +1], "%lf", &ucb_const)) {
        //         usage(argv[0]);
        //     };
        //     i += 2;}
        else if (strcmp(argv[i], "-v") == 0) {
            verbose = TRUE;
            ++i;
        } else {
            usage(argv[0]);
        }
    }
}

/*********************************************************/ /*
    Called at the beginning of a series of games
 */
void agent_init() {
    struct timeval tp;

    // generate a new random seed each time
    gettimeofday(&tp, NULL);
    srand((unsigned int)(tp.tv_usec));
}

/*********************************************************/ /*
    Called at the beginning of each game
 */
void agent_start(int this_player) {
    (void)this_player;
    //   reset_board( board );
    //   m = 0;
    //   move[m] = 0;
    //   player = this_player;
}

/*********************************************************/ /*
    Choose second move and return it
 */
int agent_second_move(int board_num, int prev_move) {
    struct timeval start, fin;

    // We are O, adjust UCB_CONST accordingly
    if (board_num == 5)
        ucb_const = UCB_CIRCLE5;
    else 
        ucb_const = UCB_CIRCLE12;
    // Internal state is represented starting from index 0.
    --board_num;
    --prev_move;
    state = initState(board_num, prev_move, -1);
    moveNo = 2;
    // Stored for tuning purposes.
    firstMove[0] = board_num + 1;
    firstMove[1] = prev_move + 1;

    gettimeofday(&start, NULL);
    int ourMove = run_mcts(state, prev_move, INITIAL_ITER);
    gettimeofday(&fin, NULL);

    uint32_t move_msec = move_msec = 1 + (fin.tv_sec - start.tv_sec) * 1000 +
                                     (fin.tv_usec - start.tv_usec) / 1000;
    totalMs += move_msec;
    double bench = (double)INITIAL_ITER / (double)move_msec;
    benchmarkedIters = bench * targetTurnTime * 1000;
    if (verbose)
        printf("trn: %d [%u ms]. %lf iters/ms\n", moveNo, move_msec, bench);
    
    stateDoMove(state, ourMove);
    return ourMove + 1;
}

/*********************************************************/ /*
    Choose third move and return it
 */
int agent_third_move(int board_num, int first_move, int prev_move) {
    struct timeval start, fin;
    // We are X, adjust UCB_CONST appropriately.
    ucb_const = UCB_CONST;
    --board_num;
    --first_move;
    --prev_move;
    moveNo = 3;
    firstMove[0] = board_num + 1;
    firstMove[1] = first_move + 1;
    state = initState(board_num, prev_move, first_move);

    gettimeofday(&start, NULL);
    int ourMove = run_mcts(state, prev_move, INITIAL_ITER);
    gettimeofday(&fin, NULL);

    uint32_t move_msec = move_msec = 1 + (fin.tv_sec - start.tv_sec) * 1000 +
                                     (fin.tv_usec - start.tv_usec) / 1000;
    totalMs += move_msec;
    double bench = (double)INITIAL_ITER / (double)move_msec;
    benchmarkedIters = bench * targetTurnTime * 1000;
    if (verbose)
        printf("trn: %d [%u ms]. %lf iters/ms\n", moveNo, move_msec, bench);

    stateDoMove(state, ourMove);
    return ourMove + 1;
}

/*********************************************************/ /*
    Choose next move and return it
 */
int agent_next_move(int prev_move) {
    struct timeval start, fin;
    --prev_move;
    moveNo += 2;
    stateDoMove(state, prev_move);

    if (moveNo > 7) {
        targetTurnTime = MAX_TARGET_TURN_TIME;
    }

    gettimeofday(&start, NULL);
    int ourMove = run_mcts(state, prev_move, benchmarkedIters);
    gettimeofday(&fin, NULL);

    uint32_t move_msec = move_msec = 1 + (fin.tv_sec - start.tv_sec) * 1000 +
                                     (fin.tv_usec - start.tv_usec) / 1000;
    totalMs += move_msec;
    double bench = (double)MAXITER / (double)move_msec;

    if (verbose)
        printf("trn: %d [%u ms]. %lf iters/ms\n", moveNo, move_msec, bench);

    // Don't want to increase iters too much towards endgame since game is
    // pretty much decided and turns are blazing fast.
    if ((bench * targetTurnTime * 1000) < MAXITER) {
        benchmarkedIters = bench * targetTurnTime * 1000;
    } else {
        benchmarkedIters = MAXITER;
    }

    stateDoMove(state, ourMove);
    return (ourMove + 1);
}

/*********************************************************/ /*
    Receive last move and mark it on the board
 */
void agent_last_move(int prev_move) {
    ++moveNo;
    (void)prev_move;
}

/*********************************************************/ /*
    Called after each game
 */
void agent_gameover(int result,                             // WIN, LOSS or DRAW
                    int cause  // TRIPLE, ILLEGAL_MOVE, TIMEOUT or FULL_BOARD
) {
    const char resultMap[3] = {'W', 'L', 'D'}; 
    const char meMap[2] = {'O', 'X'};

    // ucb_const,result,me,firstmove,turns,time
    printf("%lf,%c,%c,%d.%d,%d,%u\n",
           ucb_const, resultMap[result - WIN], meMap[state->me - CIRCLE_PLAYER],
           firstMove[0], firstMove[1], moveNo, totalMs);
    free(state);
    (void)cause;
}

/*********************************************************/ /*
    Called after the series of games
 */
void agent_cleanup() {
    // nothing to do here
}
