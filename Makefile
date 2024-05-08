CC=g++
FLAGS=-std=c++17 -g -I. -c
#FLAGS=-std=c++17 -O3 -I. -c
DEPS=game.h minimax.h player.h tree.h
OBJS=player.o main.o tic_tac_toe.o meta_tic_tac_toe.o game.o
minimax: $(HEADER) $(OBJS)
	$(CC) -o minimax $(OBJS)
clean:
	rm -f *.o
player.o: player.cpp player.h
	$(CC) $(FLAGS) player.cpp
main.o: main.cpp game.h $(DEPS) game.h tic_tac_toe.h meta_tic_tac_toe.h
	$(CC) $(FLAGS) main.cpp
tic_tac_toe.o: tic_tac_toe.cpp $(DEPS) tic_tac_toe.h
	$(CC) $(FLAGS) tic_tac_toe.cpp
meta_tic_tac_toe.o: meta_tic_tac_toe.cpp $(DEPS) meta_tic_tac_toe.h tic_tac_toe.h
	$(CC) $(FLAGS) meta_tic_tac_toe.cpp
game.o: game.cpp $(DEPS)
	$(CC) $(FLAGS) game.cpp
