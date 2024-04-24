CC=g++
tic_tac_toe: tic_tac_toe.cpp minimax.h
	$(CC) -std=c++17 tic_tac_toe.cpp -I. -o tic_tac_toe 
