#include "meta_tic_tac_toe.h"
#include "raylib_interface.h"

#include "raylib.h"

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#include <iostream>
using namespace std;

const float board_width = 600;
const float panel_width = 200;
const float panel_spacer = 10;
const float panel_x = board_width + panel_spacer;
const float text_box_height = 30;

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

namespace tic_tac_toe {

const float cell_size = board_width / 3;
DeepRule rule;
optional< Move > last_move;
vector< Move > valid_moves = rule.generate_moves();

Player current_player = player1;

void draw_board( Player* board, optional< Move > const& last_move )
{
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++)
        {
            const int idx = i * n + j;
            const Player player = board[idx];
            const Color player_color = last_move == idx ? RED : BLACK;
            draw_box( i, j, BLACK, cell_size);
            draw_player(player, i, j, player_color, cell_size);
        }
}

Move cell_indices_to_move( pair< int, int > const& cell_indices )
{
    return cell_indices.first * n + cell_indices.second;
}

} // namespace tic_tac_toe {

namespace meta_tic_tac_toe {

const float outer_cell_size = board_width / 3;
const float inner_cell_size = outer_cell_size / 3;
Rule rule;
optional< Move > last_move;
vector< Move > valid_moves = rule.generate_moves();
Player current_player = player1;

void draw_board( Rule const& rule, optional< Move > const& last_move )
{
    int idx = 0;
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++)
        {
            const bool terminal = rule.terminals[i * n + j];

            draw_box( i, j, BLACK, outer_cell_size, 0, 0, 2 );
            const int pos_x = j * outer_cell_size;
            const int pos_y = i * outer_cell_size;
            for (int i2 = 0; i2 < n; i2++)
                for (int j2 = 0; j2 < n; j2++)
                {
                    draw_box( i2, j2, BLACK, inner_cell_size, pos_x, pos_y, 1.0);
                    const Player player = rule.board[idx];
                    const Color LIGHTRED = (Color){ 255, 127, 127, 255 };
                    const Color player_color = 
                        last_move == idx ? (terminal ? LIGHTRED : RED) : (terminal ? LIGHTGRAY : BLACK);
                    ++idx;
                    draw_player(player, i2, j2, player_color, inner_cell_size, pos_x, pos_y);
                }
            
            if (terminal)
                draw_player( rule.meta_board[i * n + j], i, j, BLACK, outer_cell_size, 0, 0);
        }
}

Move cell_indices_to_move( pair< int, int > const& cell_indices )
{
    const div_t x_coord = div( cell_indices.second, n );
    const div_t y_coord = div( cell_indices.first, n );
    return y_coord.quot * n * n * n + y_coord.rem * n + x_coord.quot * n * n + x_coord.rem;
}

} // namespace meta_tic_tac_toe {

bool show_dropdown_menu( string const& items, int& selected,  bool& dropped_down, float& panel_y)
{   
    const Rectangle rect = { panel_x, panel_y, panel_width - 2 * panel_spacer, text_box_height };
    panel_y += text_box_height + panel_spacer;

    if (GuiDropdownBox( rect, items.c_str(), &selected, dropped_down))
        dropped_down = !dropped_down;
    return dropped_down;
}

bool show_button( string const& label, float& panel_y)
{
    const Rectangle rect = { panel_x, panel_y, panel_width - 2 * panel_spacer, text_box_height };
    panel_y += text_box_height + panel_spacer;

    return GuiButton( rect, label.c_str());
}

void show_label( string const& label, string const& text, float& panel_y)
{
    Rectangle rect = { panel_x, panel_y, panel_width - 2 * panel_spacer, text_box_height };
    GuiGroupBox( rect, label.c_str());
    rect.x += (panel_width - 2 * panel_spacer - GetTextWidth( text.c_str())) / 2;
    GuiLabel( rect, text.c_str());

    panel_y += text_box_height + panel_spacer;
}

string make_menu_item_list( const char* names[], size_t num_names)
{
    string items;
    for (size_t i = 0; i != num_names; ++i)
    {
        items += names[i];
        if (i != num_names - 1)
            items += ";"; 
    }
    return items;
}

namespace game_menu {

enum Selection { ttt = 0, uttt = 1 };
const char* names[] = { "tic tac toe", "ultimate tic tac toe" };
int selected = 0;
bool dropped_down = false;
const string items = make_menu_item_list( names, sizeof( names ) / sizeof( char* ));

} // namespace game_menu {

namespace mode_menu {

enum Selection { play = 0, edit = 1 };
const char* names[] = { "play", "edit" };
int selected = 0;
bool dropped_down = false;
const string items = make_menu_item_list( names, sizeof( names ) / sizeof( char* ));

} // namespace mode_menu {

namespace player_button {
string label;
Player* current_player = &tic_tac_toe::current_player;
} // namespace player_button {

optional< uint8_t > handle_board_event( 
    Player const* board, vector< uint8_t > const& valid_moves, Player player, Convert convert, int number_of_cells )  
{
    auto cell_indices = get_cell_indices( number_of_cells );
    if (!cell_indices)
        return {};
    const float cell_size = board_width / number_of_cells;
    const uint8_t move = convert(*cell_indices);
    draw_box( cell_indices->first, cell_indices->second, LIGHTGRAY, cell_size);

    if (   mode_menu::selected == mode_menu::play 
        && find( valid_moves.begin(), valid_moves.end(), move) == valid_moves.end())
        return {};

    if (IsGestureDetected( GESTURE_TAP ))
        return move;
    else if (board[move] == not_set)
        draw_player(player, cell_indices->first, cell_indices->second, LIGHTGRAY, cell_size);

    return {};
}

void show_gui()
{
    const int window_height = board_width;
    const int window_width = board_width + panel_width;
    InitWindow(window_width, window_height, "(ultimate) tic tac toe");

    SetTargetFPS(10);

    while (!WindowShouldClose())
    {
        BeginDrawing();
        ClearBackground(RAYWHITE);

        float panel_y = panel_spacer;

        if (show_dropdown_menu( game_menu::items, game_menu::selected, game_menu::dropped_down, panel_y))
        {}
        else if (show_dropdown_menu( mode_menu::items, mode_menu::selected, mode_menu::dropped_down, panel_y))
        {}
        else if (mode_menu::selected == mode_menu::edit)
        {
            if (show_button( player_button::label, panel_y ))
                *player_button::current_player = Player( -( *player_button::current_player) );
        }
        else if (mode_menu::selected == mode_menu::play)
            show_label( "current player", player_button::label, panel_y );
        
        player_button::label = ( *player_button::current_player == player1 ? "x" : "o");

        // show board
        if (game_menu::selected == game_menu::ttt)
        {
            using namespace tic_tac_toe;
            player_button::current_player = &current_player;
            draw_board( rule.board, last_move );

            optional< Move > move = handle_board_event( 
                rule.board, valid_moves, current_player, cell_indices_to_move, n );

            if (mode_menu::selected == mode_menu::edit)
            {
                last_move.reset();
                if (move)
                {
                    if (rule.board[*move] != not_set)
                        rule.undo_move( *move, current_player );
                    else 
                        rule.apply_move( *move, current_player );
                    
                    valid_moves = rule.generate_moves();
                }
            }
            else if (mode_menu::selected == mode_menu::play)
            {
                if (move)
                {
                    last_move = move;
                    rule.apply_move( *move, current_player );
                    valid_moves = rule.generate_moves();
                    current_player = Player( -current_player );
                }
            }
        }
        else if (game_menu::selected == game_menu::uttt)
        {
            using namespace meta_tic_tac_toe;
            player_button::current_player = &current_player;
            draw_board( rule, last_move );
            optional< Move > move = handle_board_event( 
                rule.board.data(), valid_moves, current_player, cell_indices_to_move, n * n );
            if (mode_menu::selected == mode_menu::edit)
            {
                last_move.reset();
                if (move)
                {
                    if (rule.board[*move] != not_set)
                        rule.undo_move( *move, current_player );
                    else 
                        rule.apply_move( *move, current_player );
                    
                    valid_moves = rule.generate_moves();
                }
            }
            else if (mode_menu::selected == mode_menu::play)
            {
                // indicate valid outer board(s)
                for (Move move : valid_moves)
                {
                    div_t d = div( move, n * n);
                    div_t ij = div( d.quot, n );
                    draw_box( ij.quot, ij.rem, RED, outer_cell_size, 0, 0, 2 );
                }

                if (move)
                {
                    last_move = move;
                    rule.apply_move( *move, current_player );
                    valid_moves = rule.generate_moves();
                    current_player = Player( -current_player );
                }
            }
        }
        else
            throw runtime_error("invalid game menu selection");
        
        EndDrawing();
    }

    CloseWindow();
}
