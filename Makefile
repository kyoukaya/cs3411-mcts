#  Makefile
#  Nine-Board Tic-Tac-Toe
#  COMP3411/9414/9814 Artificial Intelligence
#  Alan Blair, CSE, UNSW

CC = gcc
CFLAGS = -Wall -Wextra -pedantic -O3

default: agent

agent: agent.o client.o game.o mcts.o common.h agent.h game.h mcts.h
	$(CC) $(CFLAGS) -o agent agent.o client.o game.o mcts.o -lm

servt: servt.o game.o common.h game.h agent.h
	$(CC) $(CFLAGS) -o servt servt.o game.o

randt: randt.o client.o game.o common.h agent.h game.h
	$(CC) $(CFLAGS) -o randt randt.o client.o game.o

all: servt randt agent

%o:%c common.h agent.h mcts.h
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f servt randt agent *.o
