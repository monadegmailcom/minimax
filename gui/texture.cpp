#include "texture.h"

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
    const double texture_trans_x = x * texture.width / board_width - shift_x;
    const double texture_trans_y = y * texture.height / board_height - shift_y;
    
    const double zoom_center_x = texture.width / 2 - shift_x;
    const double zoom_center_y = texture.height / 2 - shift_y;

    const double texture_zoomed_x = zoom_center_x + (texture_trans_x - zoom_center_x) / zoom;
    const double texture_zoomed_y = zoom_center_y + (texture_trans_y - zoom_center_y) / zoom;

    const double scale_x = graph_width / texture.width;
    const double scale_y = graph_height / texture.height;

    return { scale_x * texture_zoomed_x, graph_height - scale_y * texture_zoomed_y };
}

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

} // namespace gui {
