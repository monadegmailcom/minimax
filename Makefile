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

INCLUDE=-isystem$(BOOST_PATH)/include/ \
        -isystem$(RAYLIB_PATH)/src \
		-isystem$(RAYGUI_PATH)/src \
		-isystem$(GRAPHVIZ_PATH)/include 
LINK=-framework CoreVideo \
     -framework IOKit \
	 -framework Cocoa \
	 -framework GLUT \
	 -framework OpenGL \
	 -L$(RAYLIB_PATH)/src -lraylib \
	 -L$(GRAPHVIZ_PATH)/lib -lgvc -lcgraph

DEBUG=-g
RELEASE=-O3 -DNDEBUG

# don't forget to clean if you change OPT
OPT=$(DEBUG)
#OPT=$(RELEASE)

FLAGS=-std=c++17 -Wall $(OPT) $(UNIVERSAL_FLAGS) $(INCLUDE) -c

SOURCES=player.cpp main.cpp tic_tac_toe.cpp meta_tic_tac_toe.cpp tree.cpp \
		gui/raylib_interface.cpp gui/algo.cpp gui/helper.cpp gui/texture.cpp \
		gui/player.cpp gui/game.cpp
ODIR=obj
OBJS=$(patsubst %.cpp,$(ODIR)/%.o,$(SOURCES))
DEPS=$(patsubst %.cpp,$(ODIR)/%.d,$(SOURCES))
#$(info DEPS=$(DEPS))

minimax: $(OBJS)
	$(CC) $(UNIVERSAL_FLAGS) -o $(ODIR)/minimax $(OBJS) $(LINK)

$(ODIR)/%.o: %.cpp | $(ODIR)
	$(CC) $(FLAGS) -MMD -MP -c $< -o $@
$(ODIR)/gui/%.o: gui/%.cpp | $(ODIR)/gui
	$(CC) $(FLAGS) -MMD -MP -c $< -o $@

$(ODIR):
	mkdir -p $(ODIR)

$(ODIR)/gui:
	mkdir -p $(ODIR)/gui

-include $(DEPS)

clean:
	rm -f $(ODIR)/*.o $(ODIR)/gui/*.o $(ODIR)/minimax $(DEPS)
