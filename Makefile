CC=g++
tic_tac_toe: player.cpp tic_tac_toe.cpp game.h player.h tree.h minimax.h
	$(CC) -std=c++17 player.cpp tic_tac_toe.cpp -I. -o tic_tac_toe
