CC=g++
# CC=g++-11
#FLAGS=-std=c++17 -g -I. -c # debug
FLAGS=-std=c++17 -O3 -DNDEBUG -I. -c # release
RULE_DEP=rule.h player.h
NEGAMAX_DEP=negamax.h $(RULE_DEP)
MINIMAX_DEP=minimax.h $(RULE_DEP)
TREE_DEP=tree.h $(MINIMAX_DEP) $(NEGAMAX_DEP)
GAME_DEP=game.h $(TREE_DEP)
TIC_TAC_TOE_DEP=tic_tac_toe.h $(RULE_DEP)
META_TIC_TAC_TOE_DEP=meta_tic_tac_toe.h $(TIC_TAC_TOE_DEP)
MONTECARLO_DEP=montecarlo.h $(RULE_DEP)
OBJS=player.o main.o tic_tac_toe.o meta_tic_tac_toe.o game.o tree.o
minimax: $(OBJS)
	$(CC) -o minimax $(OBJS)
clean:
	rm -f *.o
player.o: player.cpp player.h
	$(CC) $(FLAGS) player.cpp
main.o: main.cpp $(META_TIC_TAC_TOE_DEP) $(MONTECARLO_DEP) $(GAME_DEP)
	$(CC) $(FLAGS) main.cpp
tic_tac_toe.o: tic_tac_toe.cpp $(TIC_TAC_TOE_DEP)
	$(CC) $(FLAGS) tic_tac_toe.cpp
meta_tic_tac_toe.o: meta_tic_tac_toe.cpp $(META_TIC_TAC_TOE_DEP)
	$(CC) $(FLAGS) meta_tic_tac_toe.cpp
game.o: game.cpp $(GAME_DEP)
	$(CC) $(FLAGS) game.cpp
tree.o: tree.cpp $(TREE_DEP)
	$(CC) $(FLAGS) tree.cpp
