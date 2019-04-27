/*********************************************************
 *  agent.c
 *  Nine-Board Tic-Tac-Toe Agent
 *  COMP3411/9414/9814 Artificial Intelligence
 *  Alan Blair, CSE, UNSW
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

// int board[10][10];
// int move[MAX_MOVE+1];
// int player;
// int m;
State *state;
int moveNo = 0;
int benchmarkedIters = 0;

/*********************************************************/ /*
    Print usage information and exit
 */
void usage(char argv0[]) {
    printf("Usage: %s\n", argv0);
    printf("       [-p port]\n");  // tcp port
    printf("       [-h host]\n");  // tcp host
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
    --board_num;
    --prev_move;
    state = initState(board_num, prev_move, -1);
    moveNo = 1;

    gettimeofday(&start, NULL);
    int ourMove = run_mcts(state, prev_move, INITIAL_ITER);
    gettimeofday(&fin, NULL);

    uint32_t move_msec = move_msec = 1 + (fin.tv_sec - start.tv_sec) * 1000 +
                                     (fin.tv_usec - start.tv_usec) / 1000;
    double bench = (double)INITIAL_ITER / (double)move_msec;
    benchmarkedIters = bench * 5 * 1000;
    printf("trn: %d [%u ms]. %lf iters/ms\n", moveNo, move_msec, bench);

    stateDoMove(state, ourMove);
    return ourMove + 1;
}

/*********************************************************/ /*
    Choose third move and return it
 */
int agent_third_move(int board_num, int first_move, int prev_move) {
    struct timeval start, fin;
    --board_num;
    --first_move;
    --prev_move;
    moveNo = 2;
    state = initState(board_num, prev_move, first_move);

    gettimeofday(&start, NULL);
    int ourMove = run_mcts(state, prev_move, INITIAL_ITER);
    gettimeofday(&fin, NULL);

    uint32_t move_msec = move_msec = 1 + (fin.tv_sec - start.tv_sec) * 1000 +
                                     (fin.tv_usec - start.tv_usec) / 1000;
    double bench = (double)INITIAL_ITER / (double)move_msec;
    benchmarkedIters = bench * 5 * 1000;
    printf("trn: %d [%u ms]. %lf iters/ms\n", moveNo, move_msec, bench);

    stateDoMove(state, ourMove);
    return (ourMove + 1);
}

/*********************************************************/ /*
    Choose next move and return it
 */
int agent_next_move(int prev_move) {
    struct timeval start, fin;
    --prev_move;
    ++moveNo;
    stateDoMove(state, prev_move);

    gettimeofday(&start, NULL);
    int ourMove = run_mcts(state, prev_move, benchmarkedIters);
    gettimeofday(&fin, NULL);

    uint32_t move_msec = move_msec = 1 + (fin.tv_sec - start.tv_sec) * 1000 +
                                     (fin.tv_usec - start.tv_usec) / 1000;
    double bench = (double)MAXITER / (double)move_msec;

    printf("trn: %d [%u ms]. %lf iters/ms\n", moveNo, move_msec, bench);

    // Don't want to increase iters too much towards endgame since game is
    // pretty much decided and turns are blazing fast.
    if ((bench * 5 * 1000) < MAXITER) {
        benchmarkedIters = bench * 5 * 1000;
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
    (void)prev_move;
    // m++;
    // move[m] = prev_move;
    // board[move[m-1]][move[m]] = !player;
}

/*********************************************************/ /*
    Called after each game
 */
void agent_gameover(int result,                             // WIN, LOSS or DRAW
                    int cause  // TRIPLE, ILLEGAL_MOVE, TIMEOUT or FULL_BOARD
) {
    (void)result;
    (void)cause;
    // nothing to do here
}

/*********************************************************/ /*
    Called after the series of games
 */
void agent_cleanup() {
    free(state);
    // nothing to do here
}
