#include "raylib.h"

#include "../tree.h"

class GraphvizTree;
struct RenderData;

namespace gui {

class RaylibTexture
{
private:
    Texture2D texture;
    RenderTexture2D render_texture;
    float graph_width;
    float graph_height;
public:
    RaylibTexture( RenderData const& );

    ~RaylibTexture();

    std::pair< double, double > calc_coord( 
        float board_width, float board_height, float shift_x, float shift_y, float zoom, double x, double y );

    void draw( 
        float board_width, float board_height, float shift_x, float shift_y, float zoom );
};

} // namespace gui {
