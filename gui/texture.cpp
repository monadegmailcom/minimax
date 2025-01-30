#include "texture.h"

using namespace std;

namespace gui {

RaylibTexture::RaylibTexture( RenderData const& render_data ) 
    : graph_width( render_data.width ), graph_height( render_data.height )
{
    Image image = LoadImageFromMemory(".png", (unsigned char*)render_data.data, render_data.length);
    if (image.data == NULL) 
        throw std::runtime_error("Failed to load image from memory");
    
    texture = LoadTextureFromImage(image);
    SetTextureFilter(texture, TEXTURE_FILTER_BILINEAR);
    UnloadImage(image);     
}

RaylibTexture::~RaylibTexture()
{
    UnloadTexture( texture );
    UnloadRenderTexture(render_texture);
}

std::pair< double, double > RaylibTexture::calc_coord( 
    float board_width, float board_height, float shift_x, float shift_y, float zoom, double x, double y )
{
    // convert origin from left top to left bottom
    y = board_height - y;
    shift_y = -shift_y;

    float scale;
    if (graph_width / graph_height > board_width / board_height)
    {
        scale = graph_width / board_width;
        y -= (board_height - graph_height / scale) / 2;

        // shift is in texture coordinates
        shift_x *= graph_width / texture.width / zoom;
        shift_y *= graph_width / texture.width / zoom;
    }    
    else    
    {
        scale = graph_height / board_height;
        x -= (board_width - graph_width / scale) / 2;

        // shift is in texture coordinates
        shift_x *= graph_height / texture.height / zoom;
        shift_y *= graph_height / texture.height / zoom;
    }

    const double graph_trans_x = x * scale - shift_x;
    const double graph_trans_y = y * scale - shift_y;
    
    const double zoom_center_x = graph_width / 2 - shift_x;
    const double zoom_center_y = graph_height / 2 - shift_y;

    const double graph_zoomed_x = zoom_center_x + (graph_trans_x - zoom_center_x) / zoom;
    const double graph_zoomed_y = zoom_center_y + (graph_trans_y - zoom_center_y) / zoom;

    return { graph_zoomed_x, graph_zoomed_y };
}

void RaylibTexture::draw( 
    float board_width, float board_height, float shift_x, float shift_y, float zoom )
{
    UnloadRenderTexture(render_texture);
    float scale;
    if (graph_width / graph_height > board_width / board_height)
        scale = texture.width / board_width;
    else
        scale = texture.height / board_height;

    render_texture = LoadRenderTexture(scale * board_width, scale * board_height);
    SetTextureFilter(render_texture.texture, TEXTURE_FILTER_BILINEAR);
    BeginTextureMode( render_texture );
        ClearBackground(BLANK);
        Camera2D camera = { 
            .offset = { scale * board_width / 2, scale * board_height / 2 },
            .target = { -shift_x / zoom + texture.width / 2.0f, shift_y / zoom + texture.height / 2.0f },
            .rotation = 0.0f,
            .zoom = zoom};
        BeginMode2D( camera );
            Rectangle sourceRec = { 0, 0, (float)texture.width, (float)-texture.height };
            Rectangle destRec = { 0, 0, (float)texture.width, (float)texture.height };
            Vector2 origin = { 0, 0 };
            DrawTexturePro(texture, sourceRec, destRec, origin, 0.0f, RAYWHITE);
        EndMode2D();
    EndTextureMode();

    DrawTexturePro(
        render_texture.texture,
        (Rectangle){ 0, 0, (float)render_texture.texture.width, (float)render_texture.texture.height },
        (Rectangle){ 0, 0, board_width, board_height },
        (Vector2){ 0, 0 },
        0.0f,
        RAYWHITE
    );
}
/*
void RaylibTexture::draw( 
    float board_width, float board_height, float shift_x, float shift_y, float zoom )
{
    UnloadRenderTexture(render_texture);
    float scale = texture.width / board_width;
    render_texture = LoadRenderTexture(scale * board_width, scale * board_height);
    SetTextureFilter(render_texture.texture, TEXTURE_FILTER_BILINEAR);
    BeginTextureMode( render_texture );
        ClearBackground(BLANK);
        Camera2D camera = { 
            .offset = { scale * board_width / 2, scale * board_height / 2 },
            .target = { -shift_x + texture.width / 2.0f, shift_y + texture.height / 2.0f },
            .rotation = 0.0f,
            .zoom = zoom * scale * board_width / texture.width};
        BeginMode2D( camera );
            Rectangle sourceRec = { 0, 0, (float)texture.width, (float)-texture.height };
            Rectangle destRec = { 0, 0, (float)texture.width, (float)texture.height };
            Vector2 origin = { 0, 0 };
            DrawTexturePro(texture, sourceRec, destRec, origin, 0.0f, RAYWHITE);
        EndMode2D();
    EndTextureMode();

    DrawTexturePro(
        render_texture.texture,
        (Rectangle){ 0, 0, (float)render_texture.texture.width, (float)render_texture.texture.height },
        (Rectangle){ 0, 0, board_width, board_height },
        (Vector2){ 0, 0 },
        0.0f,
        RAYWHITE
    );
}
*/
} // namespace gui {
