#include "../meta_tic_tac_toe.h"
#include "game.h"

#include <iostream>
#include <string>
#include <cassert>
#include <thread>
#include <future>
#include <cmath>
#include <sstream>
#include <iomanip>

using namespace std;

namespace gui {

struct RaylibRender
{
    RaylibRender()
    {
        BeginDrawing();
        ClearBackground(RAYWHITE);
    }

    ~RaylibRender()
    {
        EndDrawing();
    }
};

struct RaylibInit
{
    RaylibInit()
    {
        SetConfigFlags(FLAG_WINDOW_RESIZABLE);

        InitWindow( window_width, window_height, "(ultimate) tic tac toe");
        SetWindowMinSize( window_width, window_height );
        SetTargetFPS(10);
        SetTraceLogLevel( LOG_WARNING );
    }

    ~RaylibInit()
    {
        CloseWindow();
    }
};

void handleWindowResize()
{
    const float height = GetScreenHeight();
    const float width = GetScreenWidth();

    if (height != window_height)
    {
        window_height = height;
        window_width = height + panel_width;
    }
    else if (width != window_width)
    {
        window_width = width;
        window_height = width - panel_width;
    }
    else 
        return;

    SetWindowSize( window_width, window_height );
    board_width = window_height;
    panel_x = board_width + panel_spacer;
}

void show()
{
    gv_gvc = gvContext();
    
    RaylibInit raylib_init;

    games[TicTacToeIdx].reset( new gui::TicTacToe());
    games[UltimateTicTacToeIdx].reset( new gui::MetaTicTacToe());
    
    while (!WindowShouldClose())
    {
        RaylibRender raylib_render;

        handleWindowResize();

        gui::Game& game = *games[game_menu.selected];
        game.show_side_panel();
        game.show_main_panel();
    }

    gvFreeContext(gv_gvc);
}

} // namespace gui {
