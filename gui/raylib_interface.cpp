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

// from stb_image_write.h
extern "C" unsigned char *stbi_write_png_to_mem(const unsigned char *pixels, int stride_bytes, int x, int y, int n, int *out_len);

using namespace std;

namespace gui {

// returned pointer has to be freed by the caller with free()
unsigned char* draw_board_png( void (*draw)(), float png_width, int& png_size )
{
    RenderTexture2D render_texture = LoadRenderTexture(png_width, png_width);
    SetTextureFilter(render_texture.texture, TEXTURE_FILTER_BILINEAR);
    BeginTextureMode(render_texture);
        ClearBackground(RAYWHITE);
        draw();
    EndTextureMode();

    // Save the render texture as PNG in memory
    Image image = LoadImageFromTexture(render_texture.texture);
    unsigned char* png_data = stbi_write_png_to_mem(
        (unsigned char*)image.data, 
        image.width * 4, 
        image.width, 
        image.height, 
        4, 
        &png_size
    );
    UnloadImage(image);

    return png_data;
}

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
