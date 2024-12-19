# or use gcc with CC=g++-11
CC=g++

# install some stuff with homebrew, clone raylib and raygui repo in the same directory level as this repo
# build raylib with: make PLATFORM=PLATFORM_DESKTOP
HOMEBREW=/opt/homebrew/Cellar

BOOST_PATH=$(HOMEBREW)/boost/1.85.0
GRAPHVIZ_PATH=$(HOMEBREW)/graphviz/12.2.1

PROJECT_ROOT=$(shell pwd)

# use for raylib from source
RAYLIB_PATH=$(PROJECT_ROOT)/../raylib
RAYGUI_PATH=$(PROJECT_ROOT)/../raygui

# use for universal binary does not work at the moment because glfw has to be 
# compiled with the same flags
#UNIVERSAL_FLAGS = -arch arm64 -arch x86_64
UNIVERSAL_FLAGS=

INCLUDE=-I$(BOOST_PATH)/include/ -isystem$(RAYLIB_PATH)/src -isystem$(RAYGUI_PATH)/src -I$(GRAPHVIZ_PATH)/include 
LINK=-framework CoreVideo -framework IOKit -framework Cocoa -framework GLUT -framework OpenGL -L$(RAYLIB_PATH)/src -lraylib -L$(GRAPHVIZ_PATH)/lib -lgvc -lcgraph
DEBUG=-g
RELEASE=-O3 -DNDEBUG
# don't forget to clean if you change OPT
OPT=$(DEBUG)
#OPT=$(RELEASE)
FLAGS=-std=c++17 -Wall $(OPT) $(UNIVERSAL_FLAGS) $(INCLUDE) -c
RULE_DEP=rule.h player.h
NEGAMAX_DEP=negamax.h $(RULE_DEP)
MINIMAX_DEP=minimax.h $(RULE_DEP)
MONTECARLO_DEP=montecarlo.h $(RULE_DEP)
TREE_DEP=tree.h $(MINIMAX_DEP) $(NEGAMAX_DEP) $(MONTECARLO_DEP)
GAME_DEP=game.h $(TREE_DEP)
TIC_TAC_TOE_DEP=tic_tac_toe.h $(RULE_DEP)
META_TIC_TAC_TOE_DEP=meta_tic_tac_toe.h $(TIC_TAC_TOE_DEP)
HELPER_DEP=gui/helper.h
TEXTURE_DEP=gui/texture.h $(TREE_DEP)
GUI_ALGO_DEP=gui/algo.h $(GAME_DEP) 
GUI_PLAYER_DEP=gui/player.h $(HELPER_DEP)
GUI_GAME_DEP=gui/game.h $(GUI_PLAYER_DEP)
RAYLIB_DEP=gui/raylib_interface.h $(HELPER_DEP) $(TEXTURE_DEP) $(META_TIC_TAC_TOE_DEP) $(GAME_DEP)
SOURCES=player.cpp main.cpp tic_tac_toe.cpp meta_tic_tac_toe.cpp tree.cpp gui/raylib_interface.cpp gui/algo.cpp gui/helper.cpp gui/texture.cpp gui/player.cpp gui/game.cpp
ODIR=obj
OBJS=$(patsubst %.cpp,$(ODIR)/%.o,$(SOURCES))

minimax: $(OBJS)
	$(CC) $(UNIVERSAL_FLAGS) -o $(ODIR)/minimax $(OBJS) $(LINK)
$(ODIR)/player.o: player.cpp player.h
	$(CC) $(FLAGS) -o $@ $<
$(ODIR)/main.o: main.cpp $(META_TIC_TAC_TOE_DEP) $(MONTECARLO_DEP) $(GAME_DEP) $(RAYLIB_DEP)
	$(CC) $(FLAGS) -o $@ $<
$(ODIR)/gui/algo.o: gui/algo.cpp $(GUI_ALGO_DEP)
	$(CC) $(FLAGS) -o $@ $<
$(ODIR)/gui/player.o: gui/player.cpp $(GUI_PLAYER_DEP) $(GUI_ALGO_DEP)
	$(CC) $(FLAGS) -o $@ $<
$(ODIR)/gui/helper.o: gui/helper.cpp $(HELPER_DEP)
	$(CC) $(FLAGS) -o $@ $<
$(ODIR)/gui/texture.o: gui/texture.cpp $(TEXTURE_DEP)
	$(CC) $(FLAGS) -o $@ $<
$(ODIR)/gui/game.o: gui/game.cpp $(GUI_GAME_DEP)
	$(CC) $(FLAGS) -o $@ $<
$(ODIR)/gui/raylib_interface.o: gui/raylib_interface.cpp $(RAYLIB_DEP)
	$(CC) $(FLAGS) -o $@ $<
$(ODIR)/tic_tac_toe.o: tic_tac_toe.cpp $(TIC_TAC_TOE_DEP)
	$(CC) $(FLAGS) -o $@ $<
$(ODIR)/meta_tic_tac_toe.o: meta_tic_tac_toe.cpp $(META_TIC_TAC_TOE_DEP)
	$(CC) $(FLAGS) -o $@ $<
$(ODIR)/tree.o: tree.cpp $(TREE_DEP)
	$(CC) $(FLAGS) -o $@ $<
clean:
	rm -f $(ODIR)/*.o $(ODIR)/gui/*.o
	rm -f $(ODIR)/minimax
