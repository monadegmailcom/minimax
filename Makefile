CC=g++
# or use gcc with CC=g++-11
# install boost and raylib with homebrew, clone raygui repo in the same directory as this repo
# switch to tag 4.0 of raygui repo to be compatible with raylib 5.0
HOMEBREW=/opt/homebrew/Cellar
RAYLIB_PATH=$(HOMEBREW)/raylib/5.0
BOOST_PATH=$(HOMEBREW)/boost/1.85.0
INCLUDE=-I$(BOOST_PATH)/include/ -I$(RAYLIB_PATH)/include -isystem../raygui/src
LINK=-L$(RAYLIB_PATH)/lib -lraylib
COMPILE_FLAGS=-Wall -Wpedantic
DEBUG=-g $(COMPILE_FLAGS)
RELEASE=-O3 $(COMPILE_FLAGS) -DNDEBUG
# don't forget to clean if you change OPT
OPT=$(DEBUG)
#OPT=$(RELEASE)
FLAGS=-std=c++17 -Wall $(OPT) $(INCLUDE) -c
RULE_DEP=rule.h player.h
NEGAMAX_DEP=negamax.h $(RULE_DEP)
MINIMAX_DEP=minimax.h $(RULE_DEP)
MONTECARLO_DEP=montecarlo.h $(RULE_DEP)
TREE_DEP=tree.h $(MINIMAX_DEP) $(NEGAMAX_DEP) $(MONTECARLO_DEP)
GAME_DEP=game.h $(TREE_DEP)
TIC_TAC_TOE_DEP=tic_tac_toe.h $(RULE_DEP)
META_TIC_TAC_TOE_DEP=meta_tic_tac_toe.h $(TIC_TAC_TOE_DEP)
RAYLIB_DEP=raylib_interface.h $(META_TIC_TAC_TOE_DEP)
SOURCES=player.cpp main.cpp tic_tac_toe.cpp meta_tic_tac_toe.cpp tree.cpp raylib_interface.cpp
ODIR=obj
OBJS=$(patsubst %.cpp,$(ODIR)/%.o,$(SOURCES))

minimax: $(OBJS)
	$(CC) -o $(ODIR)/minimax $(OBJS) $(LINK)
$(ODIR)/player.o: player.cpp player.h
	$(CC) $(FLAGS) -o $@ $<
$(ODIR)/main.o: main.cpp $(META_TIC_TAC_TOE_DEP) $(MONTECARLO_DEP) $(GAME_DEP) $(RAYLIB_DEP)
	$(CC) $(FLAGS) -o $@ $<
$(ODIR)/raylib_interface.o: raylib_interface.cpp $(RAYLIB_DEP)
	$(CC) $(FLAGS) -o $@ $<
$(ODIR)/tic_tac_toe.o: tic_tac_toe.cpp $(TIC_TAC_TOE_DEP)
	$(CC) $(FLAGS) -o $@ $<
$(ODIR)/meta_tic_tac_toe.o: meta_tic_tac_toe.cpp $(META_TIC_TAC_TOE_DEP)
	$(CC) $(FLAGS) -o $@ $<
$(ODIR)/tree.o: tree.cpp $(TREE_DEP)
	$(CC) $(FLAGS) -o $@ $<
clean:
	rm -f $(ODIR)/*.o
