#include "meta_tic_tac_toe.h"
#include "game.h"
#include "tree.h"

// for some reasons this has to be defined
#define RAYGUI_IMPLEMENTATION
#include "gui/raygui.h"

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
using namespace placeholders;

namespace ttt = tic_tac_toe;
namespace uttt = meta_tic_tac_toe;

float board_width = 600;
const float panel_spacer = 10;
float panel_width = 200;
float panel_x = board_width + panel_spacer;
float panel_y = 0;
const float text_box_height = 30;
float window_height = board_width;
float window_width = board_width + panel_width;

enum UIState { ConfigureGame, PlayGame };

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

void draw_player(
    Player player, int i, int j, Color color, float cell_size, float pos_x = 0, float pos_y = 0)
{
    const float spacer = cell_size / 4;
    const float line_width = cell_size / 20;

    pos_x += j * cell_size;
    pos_y += i * cell_size;

    if (player == player1)
    {    
        DrawLineEx(
            {pos_x + spacer, pos_y + spacer}, 
            {pos_x + cell_size - spacer, pos_y + cell_size - spacer}, 
            line_width, color);
        DrawLineEx(
            {pos_x + cell_size - spacer, pos_y + spacer}, 
            {pos_x + spacer, pos_y + cell_size - spacer}, 
            line_width, color);
    }
    else if (player == player2)
    {
        DrawRing( 
            {pos_x + cell_size / 2, pos_y + cell_size / 2}, 
            cell_size / 2 - spacer - line_width, cell_size / 2 - spacer, 0, 360, 36, color);
    }
}

void draw_box( 
    int i, int j, Color color, float cell_size, float pos_x = 0, float pos_y = 0, float line_width = 1)
{
    pos_x += j * cell_size;
    pos_y += i * cell_size;

    DrawRectangleLinesEx({ pos_x, pos_y, cell_size, cell_size}, line_width, color);
}

optional< pair< int, int > > get_cell_indices( int number_of_cells )
{
    const Vector2 mouse_pos = GetMousePosition();
    const float cell_size = board_width / number_of_cells;
    const int j = mouse_pos.x / cell_size;
    const int i = mouse_pos.y / cell_size;

    if (!(0 <= j && j < number_of_cells && 0 <= i && i < number_of_cells))
        return {};

    return make_pair( i, j);
}

typedef uint8_t (*Convert)( pair< int, int > const& cell_indices );

struct Menu 
{
    Menu( char const* name, vector< char const* > const& items, int selected = 0 ) 
        : name( name ), selected( selected ), prev_selected( selected ), items( items ) 
    {
        if (!items.empty())
        {
            str = items.front();
            for (auto itr = items.begin() + 1; itr != items.end(); ++itr)
                str += ";" + string( *itr );
        }
    }

    void add_item( string const& item )
    {
        items.push_back( item.c_str());
        str += ";" + item;
    }

    bool has_changed()
    {
        if (selected != prev_selected)
        {
            prev_selected = selected;
            return true;
        }
        return false;
    }

    char const* name;
    bool dropped_down = false;
    int selected;
    int prev_selected;

    vector< char const* > items;
    string str;
};

struct ValueBoxFloat
{
    ValueBoxFloat( string const& label, string const& _text ) : label( label ), text( _text.begin(), _text.end())
    { 
        text.push_back( 0 ); 
        value = TextToFloat( _text.c_str());
    }
    string label;
    vector< char > text;
    float value;
    bool edit_mode = false;
};

struct Spinner
{
    Spinner( string const& label, int value, int min, int max ) 
        : label( label ), value( value ), min( min ), max( max ) {}
    string label;
    int value;
    int min;
    int max;
    bool edit_mode = false;
};

/* a dropdown menu has to be drawn after the elements
   below it to ensure that it is drawn on top, so we
   save the y position of the dropdown menus in a vector
   and draw them in reverse order. Additionally the gui
   state of all gui controls below except a dropped down menu
   have to be STATE_DISABLED. */
struct DropDownMenu
{
    DropDownMenu() { menus.clear(); }
    ~DropDownMenu()
    {
        for (auto itr = menus.rbegin(); itr != menus.rend(); ++itr)
        {
            Menu& menu = *itr->first;
            if (distance( itr, menus.rend() ) <= first_disabled_idx)
                GuiSetState(STATE_NORMAL);
        
            const float height = text_box_height + 2 * panel_spacer;
            Rectangle rect = { panel_x, itr->second, panel_width - 2 * panel_spacer, height };
            GuiGroupBox( rect, menu.name );
            rect.x += panel_spacer;
            rect.y += panel_spacer;
            rect.width -= 2 * panel_spacer;
            rect.height -= 2 * panel_spacer;

            if (GuiDropdownBox( rect, menu.str.c_str(), &menu.selected, menu.dropped_down))
                menu.dropped_down = !menu.dropped_down;
            if (distance( itr, menus.rend() ) <= first_disabled_idx)
                GuiSetState(STATE_DISABLED);
        }

        GuiSetState(STATE_NORMAL);
    }

    void add( Menu& menu )
    {
        if (menu.dropped_down)
        {
            GuiSetState(STATE_DISABLED);
            first_disabled_idx = min( first_disabled_idx, menus.size()) + 1;
        }

        menus.push_back( make_pair( &menu, panel_y ));
        panel_y += text_box_height + 3 * panel_spacer;
    }

    // make static so we do not have to reallocate memory each time
    // we draw the dropdown menus
    static vector< pair< Menu*, float > > menus; 
    size_t first_disabled_idx = 1000; // just a large number
};

vector< pair< Menu*, float > > DropDownMenu::menus = {};

bool show_button( const char* text )
{
    const float height = text_box_height;
    Rectangle rect = { panel_x, panel_y, panel_width - 2 * panel_spacer, height };
    panel_y += height + panel_spacer;

    return GuiButton( rect, text );
}

bool show_button( const char* label, string const& text )
{
    const float height = text_box_height + 2 * panel_spacer;
    Rectangle rect = { panel_x, panel_y, panel_width - 2 * panel_spacer, height };
    GuiGroupBox( rect, label );
    rect.x += panel_spacer;
    rect.y += panel_spacer;
    rect.width -= 2 * panel_spacer;
    rect.height -= 2 * panel_spacer;
    panel_y += height + panel_spacer;

    return GuiButton( rect, text.c_str());
}

void show_label( const char* label, const char* text )
{
    Rectangle rect = { panel_x, panel_y, panel_width - 2 * panel_spacer, text_box_height };
    GuiGroupBox( rect, label );
    rect.x += (panel_width - 2 * panel_spacer - GetTextWidth( text )) / 2;
    GuiLabel( rect, text );

    panel_y += text_box_height + panel_spacer;
}

void show_spinner( Spinner& spinner )
{
    const float height = text_box_height + 2 * panel_spacer;
    Rectangle rect = { panel_x, panel_y, panel_width - 2 * panel_spacer, height };
    GuiGroupBox( rect, spinner.label.c_str());
    rect.x += panel_spacer;
    rect.y += panel_spacer;
    rect.width -= 2 * panel_spacer;
    rect.height -= 2 * panel_spacer;
    panel_y += height + panel_spacer;

    if (GuiSpinner( rect, "", &spinner.value, spinner.min, spinner.max, spinner.edit_mode ))
        spinner.edit_mode = !spinner.edit_mode;
}

void show_float_value_box( ValueBoxFloat& value_box )
{
    const float height = text_box_height + 2 * panel_spacer;
    Rectangle rect = { panel_x, panel_y, panel_width - 2 * panel_spacer, height };
    GuiGroupBox( rect, value_box.label.c_str());
    rect.x += panel_spacer;
    rect.y += panel_spacer;
    rect.width -= 2 * panel_spacer;
    rect.height -= 2 * panel_spacer;
    panel_y += height + panel_spacer;

    if (GuiValueBoxFloat( rect, 0, value_box.text.data(), &value_box.value, value_box.edit_mode))
        value_box.edit_mode = !value_box.edit_mode;
}

struct Panel
{
    Panel( const char* label ) : label( label ) 
    {
        panel_y += panel_spacer;

        // adjust x pos and width for the panel
        panel_x += panel_spacer;
        panel_width -= 2 * panel_spacer;
    }

    ~Panel()
    {
        // reset x pos and width
        panel_x -= panel_spacer;
        panel_width += 2 * panel_spacer;

        const float height = panel_y - y_pos;
        Rectangle rect = { panel_x, y_pos, panel_width - 2 * panel_spacer, height };
        GuiGroupBox( rect, label );
        rect.x += panel_spacer;
        rect.y += panel_spacer;
        rect.width -= 2 * panel_spacer;
        rect.height -= 2 * panel_spacer;

        panel_y += panel_spacer;
    }

    const char* const label;
    float y_pos = panel_y;
};

struct DisableGui
{
    DisableGui( bool disable = true ) : state( GuiGetState())
    { 
        if (disable)
            GuiSetState(STATE_DISABLED); 
    }
    
    ~DisableGui() 
    { 
        GuiSetState(state); 
    }

    void operator()()
    {
        GuiSetState(STATE_DISABLED);
    }

    int state; 
};

struct MouseEvent
{
    Vector2 position = { 0.0f, 0.0f };
    Vector2 last_mouse_position = { 0.0f, 0.0f };
    float scale = 1.0f;
    float last_click_time = 0.0f;
    bool dragging = false;
    bool double_click = false;
};
namespace gui {

class Game;
class TicTacToe;
class MetaTicTacToe;

}
enum GameIdx { TicTacToeIdx, UltimateTicTacToeIdx };
Menu game_menu = Menu { "game", {"tic tac toe", "ultimate tic tac toe"} };
unique_ptr< gui::Game > games[2];
UIState ui_state = ConfigureGame;
GVC_t* gv_gvc = nullptr;
MouseEvent mouse_event;

optional< uint8_t > handle_board_event( 
    ::Player const* board, vector< uint8_t > const& valid_moves, ::Player player, Convert convert, int number_of_cells )  
{
    auto cell_indices = get_cell_indices( number_of_cells );
    if (!cell_indices)
        return {};
    const float cell_size = board_width / number_of_cells;
    const uint8_t move = convert(*cell_indices);

    if (ui_state == PlayGame    
        && find( valid_moves.begin(), valid_moves.end(), move) == valid_moves.end())
        return {};

    draw_box( cell_indices->first, cell_indices->second, LIGHTGRAY, cell_size);

    if (IsGestureDetected( GESTURE_TAP ))
        return move;
    else if (board[move] == not_set)
        draw_player(player, cell_indices->first, cell_indices->second, LIGHTGRAY, cell_size);

    return {};
}

namespace gui {

class RaylibTexture
{
private:
    Texture2D texture;
    RenderTexture2D render_texture;
    float graph_width;
    float graph_height;
public:
    RaylibTexture( GraphvizTree::RenderData const& render_data ) : graph_width( render_data.width ), graph_height( render_data.height )
    {
        Image image = LoadImageFromMemory(".png", (unsigned char*)render_data.data, render_data.length);
        if (image.data == NULL) 
            throw std::runtime_error("Failed to load image from memory");
        
        texture = LoadTextureFromImage(image);
        SetTextureFilter(texture, TEXTURE_FILTER_BILINEAR);
        UnloadImage(image);     
    }

    ~RaylibTexture()
    {
        UnloadTexture( texture );
        UnloadRenderTexture(render_texture);
    }

    std::pair< double, double > calc_coord( 
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

    void draw( 
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
};

class Algo
{
public:
    Algo( ::Player player ) : player( player ) {}
    virtual ~Algo() {}
    virtual void reset()
    {
        // release resources
        tree_texture.reset();
        graphviz_tree.reset();
    }    
    virtual void build_tree( GVC_t* gv_gvc, DisplayNode display_node) = 0;
    virtual Algorithm* get_algorithm() = 0;
    virtual void show_side_panel(DropDownMenu& dropdown_menu) = 0;

    size_t get_tree_depth() const { return tree_depth; }
    void inc_tree_depth() { ++tree_depth; }
    void dec_tree_depth() { if (tree_depth > 1) --tree_depth; }
    bool has_texture() const { return tree_texture.operator bool(); }
    void draw_texture( float board_width, float board_height, float shift_x, float shift_y, float zoom )
    {
        if (tree_texture)
            tree_texture->draw( board_width, board_height, shift_x, shift_y, zoom );
    }
    void reset_texture( DisplayNode display_node, Layout layout )
    {
        if (!graphviz_tree)
            throw runtime_error( "graphviz_tree is not initialized");
        tree_texture.reset( new RaylibTexture( graphviz_tree->render_sub_graph( display_node, layout, tree_depth )));
    }
    void refocus_tree( 
        float board_width, float board_height, float shift_x, float shift_y, 
        float zoom, double x, double y, DisplayNode display_node, Layout layout )
    {
        auto coord = tree_texture->calc_coord( board_width, board_width, shift_x, shift_y, zoom,x, y);

        Agnode_t* gv_node = graphviz_tree->get_node_by_coord( coord.first, coord.second );
        if (gv_node)
        {
            graphviz_tree->set_focus_node( gv_node );
            reset_texture( display_node, layout );
        }

    }
protected:
    unique_ptr< RaylibTexture > tree_texture;
    unique_ptr< GraphvizTree > graphviz_tree;

    ::Player player;
    size_t tree_depth = 2;
};

template< typename MoveT >
class AlgoGenerics : public Algo
{
public:
    AlgoGenerics( ::Player player ) : Algo( player ) {}
    void reset()
    {
        // release resources
        Algo::reset();
        algorithm.reset(); 
    }
    void stop_algo()
    {
        if (algorithm)
        {
            // request algo to stop and wait until it finishes
            algorithm->stop();
            // release algo resource
            algorithm.reset();
        }
    }
    Algorithm* get_algorithm()
    {
        return algorithm.get();
    }

    virtual void start_game( GenericRule< MoveT >& rule ) = 0;

    unique_ptr< AlgorithmGenerics< MoveT > > algorithm;
};

template< typename MoveT >
class Human : public AlgoGenerics< MoveT >
{
public:
    Human( ::Player player ) : AlgoGenerics< MoveT >( player ) {}
};

template< typename MoveT >
class MMAlgo : public AlgoGenerics< MoveT >
{
public:
    MMAlgo( ::Player player ) : AlgoGenerics< MoveT >( player ) {}

    void show() {}
    virtual ~MMAlgo() {}
protected:
    virtual function< double (GenericRule< MoveT >&, ::Player) > get_eval_function() = 0;
    Spinner depth = Spinner( "depth", 7, 1, 15 );
};

class TicTacToeEval
{
public:
protected:
    function< double (GenericRule< ttt::Move >&, ::Player) > get_eval_function()
    {
        if (eval_menu.selected == 0)
            return [](auto& rule, auto) { return ttt::simple_estimate::eval( dynamic_cast< ttt::Rule const& >( rule )); };
        else if (eval_menu.selected == 1)
            return [](auto& rule, auto) { return ttt::trivial_estimate::eval( dynamic_cast< ttt::Rule const& >( rule )); };
        else
            throw runtime_error( "invalid ttt eval menu selection");
    }
    void show_side_panel(DropDownMenu& dropdown_menu)
    {
        dropdown_menu.add( eval_menu );
    }
    Menu eval_menu {"score heuristic", {"simple estimate", "trivial estimate"}};
};

class MetaTicTacToeEval
{
public:
protected:
    ValueBoxFloat score_weight = ValueBoxFloat( "score weight", "9.0" );
    Menu eval_menu = Menu {"score heuristic", {"simple estimate" }}; 
    function< double (GenericRule< uttt::Move >&, ::Player) > get_eval_function()
    {
        if (eval_menu.selected == 0)
            return [this](GenericRule< uttt::Move >& rule, ::Player) 
            { return uttt::simple_estimate::eval( dynamic_cast< uttt::Rule& >( rule ), score_weight.value ); };
        else
            throw runtime_error( "invalid uttt eval menu selection");
    }
    void show_side_panel(DropDownMenu& dropdown_menu)
    {
        dropdown_menu.add( eval_menu );
        show_float_value_box( score_weight );
    }
};

template< typename MoveT >
class Negamax : public MMAlgo< MoveT >
{
public:
    Negamax( ::Player player ) : MMAlgo< MoveT >( player ) {}
    void start_game( GenericRule< MoveT >& rule )
    {
        negamax_algorithm = new NegamaxAlgorithm< MoveT >(
            rule, this->player, this->depth.value, get_reorder_function(), get_eval_function());
        this->algorithm.reset( negamax_algorithm ); 
    }
    void show_side_panel(DropDownMenu& dropdown_menu)
    {
        show_spinner( this->depth );
        dropdown_menu.add( reorder_menu );
    }
    void build_tree( GVC_t* gv_gvc, DisplayNode display_node) {}
protected:
    NegamaxAlgorithm< MoveT >* negamax_algorithm = nullptr;
    Menu reorder_menu { "reorder moves", {"shuffle", "reorder by score"}, 1 };
    ReOrder< MoveT > get_reorder_function()
    {
        if (reorder_menu.selected == 0)
            return [shuffle = make_shared< Shuffle< MoveT > >()]
                (GenericRule< MoveT >& rule, auto player, auto begin, auto end) 
                { (*shuffle)( rule, player, begin, end ); };
        else if (reorder_menu.selected == 1)
            return [rbs = make_shared< ReorderByScore< MoveT > >( this->get_eval_function())]
                (auto& rule, auto player, auto begin, auto end) 
                { (*rbs)( rule, player, begin, end ); };
        else    
            throw runtime_error( "invalid reorder menu selection");
    } 

    virtual function< double (GenericRule< uttt::Move >&, ::Player) > get_eval_function() = 0;
};

class TicTacToeNegamax : public Negamax< ttt::Move >, public TicTacToeEval
{
public:
    TicTacToeNegamax( ::Player player ) : Negamax< ttt::Move >( player ) {}
protected:
    function< double (GenericRule< uttt::Move >&, ::Player) > get_eval_function()
    { return TicTacToeEval::get_eval_function(); }
    void show_side_panel(DropDownMenu& dropdown_menu)    
    {
        TicTacToeEval::show_side_panel( dropdown_menu);
    }
};

class MetaTicTacToeNegamax : public Negamax< uttt::Move >, public MetaTicTacToeEval
{
public:
    MetaTicTacToeNegamax( ::Player player ) : Negamax< uttt::Move >( player ) {}
protected:
    virtual void show_side_panel(DropDownMenu& dropdown_menu)
    {
        Negamax::show_side_panel( dropdown_menu);
        MetaTicTacToeEval::show_side_panel( dropdown_menu );
        
    }
    function< double (GenericRule< uttt::Move >&, ::Player) > get_eval_function()
    { return MetaTicTacToeEval::get_eval_function(); }
};

template< typename MoveT >
class Minimax : public MMAlgo< MoveT >
{
public:
    Minimax( ::Player player ) 
    : MMAlgo< MoveT >( player ), 
      recursion_menu( Menu { "recursion", {"max depth", "max vertices"}} ), 
      choose_menu( Menu { "choose", {"best", "epsilon bucket"}} ) {}
    void start_game( GenericRule< MoveT >& rule )
    {
        minimax_algorithm = new MinimaxAlgorithm< MoveT >(
            rule, this->player, this->get_eval_function(), get_recursion_function(), get_choose_move_function());
        this->algorithm.reset( minimax_algorithm );
    }
    void build_tree( GVC_t* gv_gvc, DisplayNode display_node) {}
protected:
    Spinner max_vertices = Spinner( "max vertices", 280000, 1, 1000000 );
    enum RecursionIdx { MaxDepthIdx, MaxVerticesIdx };
    Menu recursion_menu;
    enum ChooseIdx { BestIdx, EpsilonBucketIdx };
    Menu choose_menu;
    ValueBoxFloat bucket_width = ValueBoxFloat( "bucket width", "1.00" );
    MinimaxAlgorithm< MoveT >* minimax_algorithm = nullptr;
    
    virtual void show_side_panel(DropDownMenu& dropdown_menu)    
    {    
        dropdown_menu.add( recursion_menu );
        dropdown_menu.add( choose_menu );
        if (recursion_menu.selected == MaxVerticesIdx)
            show_spinner( max_vertices );
        else if (recursion_menu.selected == MaxDepthIdx)
            show_spinner( this->depth );
        if (choose_menu.selected == EpsilonBucketIdx)
            show_float_value_box( bucket_width );
    }
    virtual Recursion< ttt::Move >* get_recursion_function() = 0;
    virtual function< ttt::Move const& (VertexList< ttt::Move > const&) > get_choose_move_function() = 0;
};

class TicTacToeMinimax : public Minimax< ttt::Move >, public TicTacToeEval
{
public:
    TicTacToeMinimax( ::Player player ) : Minimax< ttt::Move >( player ) {}
protected:
    function< double (GenericRule< ttt::Move >&, ::Player) > get_eval_function()
    {
        return TicTacToeEval::get_eval_function();
    }
    void show_side_panel(DropDownMenu& dropdown_menu)
    {
        Minimax< ttt::Move >::show_side_panel( dropdown_menu);
        TicTacToeEval::show_side_panel( dropdown_menu );
    }

    Recursion< ttt::Move >* get_recursion_function()
    {
        if (recursion_menu.selected == 0)
            return new MaxDepth< ttt::Move >( depth.value);
        else if (recursion_menu.selected == 1)
            return new MaxVertices< ttt::Move > ( max_vertices.value );
        else
            throw runtime_error( "invalid ttt recursion menu selection");
    }

    function< ttt::Move const& (VertexList< ttt::Move > const&) > get_choose_move_function()
    {
        if (choose_menu.selected == 0)
            return bind( &ChooseFirst< ttt::Move >::operator(), ChooseFirst< ttt::Move >(), _1 );
        else if (choose_menu.selected == 1)
            return [cm = make_shared< ChooseMove< ttt::Move > >( bucket_width.value)]
                (VertexList< ttt::Move > const& vertices)
                { return (*cm)( vertices ); };
        else
            throw runtime_error( "invalid ttt choose move menu selection");
    }
};

class MetaTicTacToeMinimax : public Minimax< uttt::Move >, public MetaTicTacToeEval
{
public:
    MetaTicTacToeMinimax( ::Player player ) : Minimax< uttt::Move >( player) {}
protected:
    function< double (GenericRule< uttt::Move >&, ::Player) > get_eval_function()
    {
        return MetaTicTacToeEval::get_eval_function();
    }

    void show_side_panel(DropDownMenu& dropdown_menu)
    {
        Minimax< uttt::Move >::show_side_panel( dropdown_menu);
        MetaTicTacToeEval::show_side_panel( dropdown_menu );
    }

    Recursion< uttt::Move >* get_recursion_function()
    {
        if (recursion_menu.selected == 0)
            return new MaxDepth< uttt::Move >( depth.value);
        else if (recursion_menu.selected == 1)
            return new MaxVertices< uttt::Move >( max_vertices.value );
        else
            throw runtime_error( "invalid uttt recursion menu selection");
    }

    function< uttt::Move const& (VertexList< uttt::Move > const&) > get_choose_move_function()
    {
        if (choose_menu.selected == 0)
            return bind( &ChooseFirst< uttt::Move >::operator(), ChooseFirst< uttt::Move >(), _1 );
        else if (choose_menu.selected == 1)
            return [cm = make_shared< ChooseMove< uttt::Move > >( bucket_width.value)]
                (VertexList< uttt::Move > const& vertices)
                { return (*cm)( vertices ); };
        else
            throw runtime_error( "invalid uttt choose move menu selection");
    }
};

template< typename MoveT >
class Montecarlo : public AlgoGenerics< MoveT >
{
public:
    Montecarlo( ::Player player, Menu const& choose_menu ) 
    : AlgoGenerics< MoveT >( player ), choose_menu( choose_menu ) {}
    void start_game( GenericRule< MoveT >& rule )
    {
        this->algorithm.reset( new montecarlo::Algorithm< MoveT >(
            rule, this->player, this->get_choose_move_function(), simulations.value, 
            exploration_factor.value ));
    }
    void build_tree( GVC_t* gv_gvc, DisplayNode display_node)
    {
        if (!this->algorithm)
            throw runtime_error( "invalid algo (build_tree)");

        auto m_algo = dynamic_cast< montecarlo::Algorithm< ttt::Move >* >( this->get_algorithm());
        this->graphviz_tree.reset( new montecarlo::TicTacToeTree(
            gv_gvc, this->player, m_algo->get_mcts().exploration, m_algo->get_root()));
        this->reset_texture( display_node, Circular );
    }
protected:
    virtual montecarlo::ChooseMove< ttt::Move >* get_choose_move_function() = 0;

    void show_side_panel(DropDownMenu& dropdown_menu)
    {
        dropdown_menu.add( choose_menu );
        show_spinner( simulations );
        show_float_value_box( exploration_factor );
    }
    montecarlo::Algorithm< MoveT >* montecarlo_algorithm = nullptr;
    Menu choose_menu;
    Spinner simulations = Spinner( "simulations", 100 /*80000*/, 1, 1000000 );
    ValueBoxFloat exploration_factor = ValueBoxFloat( "exploration factor", "0.40" );
};

class TicTacToeMontecarlo : public Montecarlo< ttt::Move >
{
public: 
    TicTacToeMontecarlo( ::Player player ) 
    : Montecarlo< ttt::Move >( player, Menu { "choose", {"best"}}) {}
protected:
    montecarlo::ChooseMove< ttt::Move >* get_choose_move_function()
    {
        if (choose_menu.selected == 0)
            return new montecarlo::ChooseBest< ttt::Move >();
        else
            throw runtime_error( "invalid ttt montecarlo choose move menu selection");
    }
};

class MetaTicTacToeMontecarlo : public Montecarlo< uttt::Move >
{
public: 
    MetaTicTacToeMontecarlo( ::Player player ) 
    : Montecarlo< uttt::Move >( player, Menu { "choose", {"best"}}) {}
protected:
    montecarlo::ChooseMove< uttt::Move >* get_choose_move_function()
    {
        if (choose_menu.selected == 0)
            return new montecarlo::ChooseBest< uttt::Move >();
        else
            throw runtime_error( "invalid uttt montecarlo choose move menu selection");
    }
};

struct Player
{
    Player( string const& name, ::Player player ) : name( name ), player( player ) {}
    virtual ~Player() {}

    string name;
    ::Player player;
    static const size_t algo_count = 4;
    enum AlgoIdx { HumanIdx, NegamaxIdx, MinimaxIdx, MontecarloIdx };
    Menu algo_menu = Menu { "algorithm", {"human", "negamax", "minimax", "montecarlo"}, MontecarloIdx };

    virtual void show_game_info( bool ticking ) = 0;
    virtual Algo& get_algo() = 0;
};

template< typename MoveT >
class PlayerGenerics : public Player
{
public:
    PlayerGenerics( string const& name, ::Player player ) : Player( name, player ) {}
    AlgoGenerics< MoveT >& get_algo_generic()
    {
        return *algos[algo_menu.selected];
    }

    Algo& get_algo()
    {
        return get_algo_generic();
    }

    void show_game_info( bool ticking )
    {
        Panel panel( name.c_str());
        auto algorithm = algos[algo_menu.selected]->algorithm.get();
        if (!algorithm)
            return;
        chrono::microseconds duration = algorithm->get_duration();
        
        if (ticking)
            duration += chrono::duration_cast< std::chrono::microseconds >( 
                chrono::steady_clock::now() - algorithm->get_start_time());

        show_label( "algorithm", algo_menu.items[algo_menu.selected]);
        const ldiv_t min = ldiv( duration.count(), 60 * 1000000);
        const ldiv_t sec = ldiv( min.rem, 1000000);
        const long dsec = sec.rem / 100000;
        static ostringstream stream; // reuse memory
        stream.str( "" );
        stream << setfill( '0' ) << setw( 2 ) << min.quot << ":" << setw( 2 ) << sec.quot
                << "." << setw( 1 ) << dsec;  
        show_label( "accumulated time", stream.str().c_str());
    }
    void reset()
    {
        for (auto& algo : algos)
            algo->reset();
    }
protected:
    unique_ptr< AlgoGenerics< MoveT > > algos[algo_count];
};

class TicTacToePlayer : public PlayerGenerics< ttt::Move >
{
public:
    TicTacToePlayer( string const& name, ::Player player ) : PlayerGenerics( name, player )
    {
        algos[Player::NegamaxIdx] = make_unique< TicTacToeNegamax >( player );
        algos[Player::MinimaxIdx] = make_unique< TicTacToeMinimax >( player );
        algos[Player::MontecarloIdx] = make_unique< TicTacToeMontecarlo >( player );
    }
};

class MetaTicTacToePlayer : public PlayerGenerics< uttt::Move >
{
public:
    MetaTicTacToePlayer( string const& name, ::Player player ) : PlayerGenerics( name, player )
    {
        algos[Player::NegamaxIdx] = make_unique< MetaTicTacToeNegamax >( player );
        algos[Player::MinimaxIdx] = make_unique< MetaTicTacToeMinimax >( player );
        algos[Player::MontecarloIdx] = make_unique< MetaTicTacToeMontecarlo >( player );
    }
};

class Game 
{
public:
    virtual ~Game() {}

    enum PlayModeIdx { PlayIdx, SingleStepPlayIdx, BatchPlayIdx };
    Menu play_mode_menu = Menu { "play mode", {"play", "single step play", "batch play"}, SingleStepPlayIdx };
    bool on_hold = false;

    enum ShowIdx { BoardIdx, TreeIdx };
    Menu show_menu = Menu { "show", {"board", "tree"}, TreeIdx };

    enum DisplayIdx { PNGIdx, MoveIdx, StatsIdx};
    DisplayNode display_modes[3] = { DisplayBoard, DisplayMove, DisplayStats };
    Menu display_menu = Menu { "display", {"board", "move", "stats"}, MoveIdx };

    ::Player winner = not_set;
    Player* current_player = nullptr;
    Player* opponent = nullptr;

    virtual void reset() = 0;
    virtual void stop_game() = 0;
    virtual void start_game() = 0;
    virtual void show_board(UIState) = 0;
    virtual void show_graph( MouseEvent& ) = 0;
    virtual void show_side_panel() = 0;
};

template< typename MoveT >
class GameGenerics : public Game
{
public:
    GameGenerics( 
        PlayerGenerics< MoveT >* player1, PlayerGenerics< MoveT >* player2, GenericRule< MoveT >* rule) 
            : player1( player1 ), player2( player2 ), rule( rule )
    {
        valid_moves = rule->generate_moves();
        current_player = player1;
        opponent = player2;
    }

    virtual ~GameGenerics() {}

    virtual void reset()
    {
        player1->reset();
        player2->reset();
        last_move.reset();
        valid_moves = rule->generate_moves();
    }

    // return true if the game is finished
    bool process_move()
    {
        if (winner != not_set || valid_moves.empty())
            return true;

        if (on_hold)
            return false;

        auto current_player_algo = player1->get_algo_generic().algorithm.get();
        auto opponent_algo = player2->get_algo_generic().algorithm.get();
        if (current_player == player2.get())
            swap( current_player_algo, opponent_algo);

        if (!current_player_algo || !opponent_algo)
            throw runtime_error( "algo not set (process move)");

        optional< MoveT > move = current_player_algo->get_move();
        if (move)
        {
            if (play_mode_menu.selected == Game::SingleStepPlayIdx)
            {
                if (opponent->algo_menu.selected != Player::HumanIdx)
                    on_hold = true;

                if (current_player->algo_menu.selected == Player::MontecarloIdx)
                    current_player->get_algo().build_tree( gv_gvc, display_modes[display_menu.selected]);
            }
            
            last_move = move;
            rule->apply_move( *move, current_player->player );
            valid_moves = rule->generate_moves();

            current_player_algo->apply_move( *move );
            opponent_algo->opponent_move( *move );

            winner = rule->get_winner();
            swap( current_player, opponent );
        }

        return false;
    }

    void stop_game()
    {
        player1->get_algo_generic().stop_algo();
        player2->get_algo_generic().stop_algo();

        reset();
        winner = not_set;
    }

    void start_game()
    {
        player1->get_algo_generic().start_game( *rule );
        player2->get_algo_generic().start_game( *rule );
    }
    void show_board(UIState ui_state)
    {
        draw_board( board_width);

        if (   ui_state == PlayGame 
            && winner == not_set
            && current_player->algo_menu.selected == Player::HumanIdx)
        {
            optional< MoveT > move = get_move(); 
            if (move)
                process_play_move( *move );
        }
        else if (ui_state == ConfigureGame)
        {
            optional< MoveT > move = get_move();             
            if (move)
                process_config_move( *move );
        }
    }
    void show_graph( MouseEvent& mouse_event)
    {
        Player& player = *opponent;
        if (player.get_algo().has_texture())
        {
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) 
            {
                mouse_event.last_mouse_position = GetMousePosition();
                double click_time = GetTime();
                const float time_threshold = 0.3f;
                if (click_time - mouse_event.last_click_time < time_threshold)
                    mouse_event.double_click = true;
                else
                    mouse_event.double_click = false;

                mouse_event.last_click_time = click_time;

                if (CheckCollisionPointRec( 
                        mouse_event.last_mouse_position, 
                        {0, 0, board_width, board_width}))
                    mouse_event.dragging = true;
            }
            if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON) && mouse_event.dragging)
            {
                Vector2 current_mouse_pos = GetMousePosition();
                const float distance_threshold = 5.0f;
                // clicked and not dragged?
                if (std::abs(current_mouse_pos.x - mouse_event.last_mouse_position.x) < distance_threshold &&
                    std::abs(current_mouse_pos.y - mouse_event.last_mouse_position.y) < distance_threshold &&
                    mouse_event.double_click)
                {
                    player.get_algo().refocus_tree( 
                        board_width, board_width, 
                        mouse_event.position.x, mouse_event.position.y, mouse_event.scale,
                        current_mouse_pos.x, current_mouse_pos.y,
                        display_modes[display_menu.selected], Circular );
                    mouse_event = MouseEvent();
                }

                mouse_event.dragging = false;
            }
            
            if (mouse_event.dragging) 
            {
                const Vector2 current_mouse_position = GetMousePosition();
                const Vector2 delta = {current_mouse_position.x - mouse_event.last_mouse_position.x,
                                current_mouse_position.y - mouse_event.last_mouse_position.y };
                mouse_event.position.x += delta.x;
                mouse_event.position.y += delta.y;
                mouse_event.last_mouse_position = current_mouse_position;
            }

            const float wheel = GetMouseWheelMove();
            if (wheel != 0) 
            {
                mouse_event.scale += wheel * 0.1f;
                // Prevent zooming out too much
                if (mouse_event.scale < 0.1f) 
                    mouse_event.scale = 0.1f; 
            }

            player.get_algo().draw_texture( 
                board_width, board_width, mouse_event.position.x, mouse_event.position.y, mouse_event.scale);
        }
    }

    void show_side_panel()
    {
        DropDownMenu dropdown_menu;

        player1->get_algo().show_side_panel( dropdown_menu);
        player2->get_algo().show_side_panel( dropdown_menu);

        if (ui_state == PlayGame)
        {
            show_label( "mode", play_mode_menu.items[play_mode_menu.selected]);
            show_label( "winner", 
                winner == not_set 
                    ? "none" 
                    : winner == current_player->player 
                        ? current_player->name.c_str() 
                        : opponent->name.c_str());
        }

        panel_y = panel_spacer;

        if (ui_state == ConfigureGame)
        {
            show_label( "mode", "configure" );
            dropdown_menu.add( game_menu );
            if (show_button( "toggle player", current_player->name ))
                swap( current_player, opponent );
                    
            dropdown_menu.add( current_player->algo_menu );
            current_player->get_algo().show_side_panel( dropdown_menu);

            dropdown_menu.add( play_mode_menu );

            if (show_button( "start game" ))
            {
                start_game();
                on_hold = (   play_mode_menu.selected == SingleStepPlayIdx
                            && current_player->algo_menu.selected != Player::HumanIdx);

                ui_state = PlayGame;
            }
        }
        else if (ui_state == PlayGame)
        {
            bool game_finished = process_move();

            const char* button_text = 0;
            static string status_text;
            if (winner == ::player1)
            {
                status_text = player1->name + " wins";
                button_text = "back";
            }
            else if (winner == ::player2)
            {
                status_text = player2->name + " wins";
                button_text = "back";
            }
            else if (game_finished)
            {
                status_text = "draw";
                button_text = "back";
            }
            else
            {
                status_text = current_player->name + "'s turn";
                button_text = "abort game";
            }

            show_label( "mode", play_mode_menu.items[play_mode_menu.selected]);
            
            current_player->show_game_info( !game_finished && !on_hold );
            opponent->show_game_info( false );

            show_label( "status", status_text.c_str());

            if (play_mode_menu.selected == SingleStepPlayIdx)
            {
                dropdown_menu.add( show_menu );

                if (show_menu.selected == TreeIdx)
                {
                    const size_t prev_tree_depth = opponent->get_algo().get_tree_depth();
                    
                    {
                        DisableGui disable_gui( false );
                        if (show_button( "#121#" ))
                            opponent->get_algo().inc_tree_depth();
                        if (opponent->get_algo().get_tree_depth() <= 1)
                            disable_gui();
                        show_label( "tree depth", to_string( opponent->get_algo().get_tree_depth()).c_str());
                        if (show_button( "#120#" ))
                        {
                            if (!opponent->get_algo().get_tree_depth())
                                throw runtime_error( "invalid tree depth (show_side_panel)");
                            opponent->get_algo().dec_tree_depth();
                        }
                    }

                    dropdown_menu.add( display_menu );
                    
                    if (display_menu.has_changed() || prev_tree_depth != opponent->get_algo().get_tree_depth())
                    {
                        opponent->get_algo().reset_texture( display_modes[display_menu.selected], Circular );
                        mouse_event = MouseEvent();
                    }
                }

                {
                    DisableGui disable_gui( false );
                    if (!on_hold)
                        disable_gui();

                    if (show_button( "next move" ))
                        on_hold = false;
                }
            }
            // TODO: batch mode not yet implemented
            else if (play_mode_menu.selected == BatchPlayIdx)
            {}

            if (show_button( button_text))
            {
                stop_game();

                // reset current player to player1
                if (current_player != player1.get())
                    swap( current_player, opponent );

                player1->reset();
                player2->reset();

                mouse_event = MouseEvent();

                ui_state = ConfigureGame;
            }
        }
    }

    virtual void process_config_move( MoveT const& ) = 0;
    virtual void process_play_move( MoveT const& move ) = 0;
    virtual optional< MoveT > get_move() = 0;
protected:
    unique_ptr< PlayerGenerics< MoveT > > player1;
    unique_ptr< PlayerGenerics< MoveT > > player2;
    unique_ptr< GenericRule< MoveT > > rule;
    optional< MoveT > last_move;
    vector< MoveT > valid_moves;

    virtual void draw_board( float board_width) = 0;
};

class TicTacToe : public GameGenerics< ttt::Move >
{
public:
    TicTacToe() : GameGenerics< ttt::Move >( 
        new TicTacToePlayer( "player x", ::player1 ), 
        new TicTacToePlayer( "player o", ::player2 ),
        new ttt::DeepRule()) {}
    void draw_board( float board_width) 
    {
        ::Player const* const board = dynamic_cast< ttt::Rule* >( rule.get())->board;
        const float cell_size = board_width / 3;
        for (int i = 0; i < ttt::n; i++)
            for (int j = 0; j < ttt::n; j++)
            {
                const int idx = i * ttt::n + j;
                const ::Player player = board[idx];
                const Color player_color = last_move == idx ? RED : BLACK;
                draw_box( i, j, BLACK, cell_size);
                draw_player(player, i, j, player_color, cell_size);
            }
    }
    static u_int8_t cell_indices_to_move( pair< int, int > const& cell_indices )
    {
        return cell_indices.first * ttt::n + cell_indices.second;
    }

    optional< ttt::Move > get_move() 
    {
        return handle_board_event( 
            dynamic_cast< ttt::Rule* >( rule.get())->board, 
            valid_moves, current_player->player, cell_indices_to_move, ttt::n );
    }

    void process_play_move( ttt::Move const& move )
    {
        if (dynamic_cast< ttt::Rule* >( rule.get())->board[move] != not_set)
            rule->undo_move( move, current_player->player );
        else 
            rule->apply_move( move, current_player->player );
        
        winner = rule->get_winner();
    }

    void process_config_move( ttt::Move const& move )
    {
        auto algo = dynamic_cast< interactive::Algorithm< ttt::Move >* >( 
            current_player->get_algo().get_algorithm());
        if (!algo)
            throw runtime_error( "invalid interactive algorithm");
        algo->set_move( move );
    }
protected:
    void reset()
    {
        GameGenerics::reset();
        rule.reset( new ttt::DeepRule());
    }
};

class MetaTicTacToe : public GameGenerics< uttt::Move >
{
public:
    MetaTicTacToe() : GameGenerics< uttt::Move >( 
        new MetaTicTacToePlayer( "player x", ::player1), 
        new MetaTicTacToePlayer( "player o", ::player2 ), 
        new uttt::Rule()) {}

    void draw_board( float board_width )
    {
        uttt::Rule const& r = dynamic_cast< uttt::Rule& >( *rule );
        const float outer_cell_size = board_width / 3;
        const float inner_cell_size = outer_cell_size / 3;
        int idx = 0;
        for (int i = 0; i < uttt::n; i++)
            for (int j = 0; j < uttt::n; j++)
            {
                const bool terminal = r.terminals[i * uttt::n + j];

                draw_box( i, j, BLACK, outer_cell_size, 0, 0, 2 );
                const int pos_x = j * outer_cell_size;
                const int pos_y = i * outer_cell_size;
                for (int i2 = 0; i2 < uttt::n; i2++)
                    for (int j2 = 0; j2 < uttt::n; j2++)
                    {
                        draw_box( i2, j2, BLACK, inner_cell_size, pos_x, pos_y, 1.0);
                        const ::Player player = r.board[idx];
                        const Color LIGHTRED { 255, 127, 127, 255 };
                        const Color player_color = 
                            last_move == idx ? (terminal ? LIGHTRED : RED) : (terminal ? LIGHTGRAY : BLACK);
                        ++idx;
                        draw_player(player, i2, j2, player_color, inner_cell_size, pos_x, pos_y);
                    }
                
                if (terminal)
                    draw_player( r.meta_board[i * uttt::n + j], i, j, BLACK, outer_cell_size, 0, 0);
            }
    }
    static u_int8_t cell_indices_to_move( pair< int, int > const& cell_indices )
    {
        const div_t x_coord = div( cell_indices.second, uttt::n );
        const div_t y_coord = div( cell_indices.first, uttt::n );
        return y_coord.quot * uttt::n * uttt::n * uttt::n + y_coord.rem * uttt::n 
               + x_coord.quot * uttt::n * uttt::n + x_coord.rem;
    }

    optional< uttt::Move > get_move() 
    {
        return handle_board_event( 
            dynamic_cast< uttt::Rule* >( rule.get())->board.data(), 
            valid_moves, current_player->player, cell_indices_to_move, uttt::n * uttt::n);
    }

    void process_play_move( uttt::Move const& move )
    {
        if (dynamic_cast< uttt::Rule* >( rule.get())->board.data()[move] != not_set)
            rule->undo_move( move, current_player->player );
        else 
            rule->apply_move( move, current_player->player );
        
        winner = rule->get_winner();
    }

    void process_config_move( uttt::Move const& move )
    {
        // indicate valid outer board(s)
        for (uttt::Move move : valid_moves)
        {
            div_t d = div( move, uttt::n * uttt::n);
            div_t ij = div( d.quot, uttt::n );

            draw_box( ij.quot, ij.rem, RED, board_width / 3, 0, 0, 2 );
        }

        if (move)
        {
            auto algo = dynamic_cast< interactive::Algorithm< uttt::Move >* >( 
                current_player->get_algo().get_algorithm());
            if (!algo)
                throw runtime_error( "invalid interactive algorithm");
            algo->set_move( move );
        }
    }
protected:
    void reset()
    {
        GameGenerics::reset();
        rule.reset( new uttt::Rule());
    }
};

} // namespace gui {

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

namespace gui {

void show()
{
    gv_gvc = gvContext();
    
    RaylibInit raylib_init;

enum GameIdx { TicTacToeIdx, UltimateTicTacToeIdx };
    games[TicTacToeIdx].reset( new gui::TicTacToe());
    games[UltimateTicTacToeIdx].reset( new gui::MetaTicTacToe());

    while (!WindowShouldClose())
    {
        RaylibRender raylib_render;
        handleWindowResize();

        gui::Game& game = *games[game_menu.selected];
        game.show_side_panel();
        
        if (   ui_state == ConfigureGame
            || game.opponent->algo_menu.selected == gui::Player::HumanIdx 
            || game.show_menu.selected == gui::Game::BoardIdx)
            game.show_board( ui_state );
        else if (game.show_menu.selected == gui::Game::TreeIdx)
            game.show_graph( mouse_event );
    }

    gvFreeContext(gv_gvc);
}

} // namespace gui {
