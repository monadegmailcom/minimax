#include "meta_tic_tac_toe.h"
#include "game.h"

#include "raylib.h"

// for some reasons this has to be defined
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#include <iostream>
#include <string>
#include <cassert>
#include <thread>
#include <future>

using namespace std;

namespace ttt = tic_tac_toe;
namespace uttt = meta_tic_tac_toe;

const float board_width = 600;
const float panel_width = 200;
const float panel_spacer = 10;
const float panel_x = board_width + panel_spacer;
const float text_box_height = 30;
const int window_height = board_width;
const int window_width = board_width + panel_width;

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
DeepRule initial_rule;
optional< Move > last_move;
vector< Move > valid_moves = rule.generate_moves();

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
Rule initial_rule;
optional< Move > last_move;
vector< Move > valid_moves = rule.generate_moves();

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
                    const Color LIGHTRED { 255, 127, 127, 255 };
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

struct Menu 
{
    Menu( string const& name, vector< string > const& items, int selected = 0 ) 
        : name( name ), dropped_down( false ), selected( selected ), items( items ) 
    {
        if (!items.empty())
        {
            str = items.front();
            for (auto itr = items.begin() + 1; itr != items.end(); ++itr)
                str += ";" + *itr;
        }
    }
    string name;
    bool dropped_down;
    int selected;
    const vector< string > items;
    string str;
};

struct TextBox
{
    TextBox( string const& label, string const& _text ) : label( label ), text( _text.begin(), _text.end())
    { text.push_back( 0 ); }
    string label;
    vector< char > text;
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
   state of all gui controls except a dropped down menu
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
            GuiGroupBox( rect, menu.name.c_str());
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

    void add( Menu& menu, float& panel_y )
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

bool show_button( string const& text, float& panel_y)
{
    const float height = text_box_height;
    Rectangle rect = { panel_x, panel_y, panel_width - 2 * panel_spacer, height };
    panel_y += height + panel_spacer;

    return GuiButton( rect, text.c_str());
}

bool show_button( string const& label, string const& text, float& panel_y)
{
    const float height = text_box_height + 2 * panel_spacer;
    Rectangle rect = { panel_x, panel_y, panel_width - 2 * panel_spacer, height };
    GuiGroupBox( rect, label.c_str());
    rect.x += panel_spacer;
    rect.y += panel_spacer;
    rect.width -= 2 * panel_spacer;
    rect.height -= 2 * panel_spacer;
    panel_y += height + panel_spacer;

    return GuiButton( rect, text.c_str());
}

void show_label( string const& label, string const& text, float& panel_y)
{
    Rectangle rect = { panel_x, panel_y, panel_width - 2 * panel_spacer, text_box_height };
    GuiGroupBox( rect, label.c_str());
    rect.x += (panel_width - 2 * panel_spacer - GetTextWidth( text.c_str())) / 2;
    GuiLabel( rect, text.c_str());

    panel_y += text_box_height + panel_spacer;
}

void show_spinner( Spinner& spinner, float& panel_y)
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

void show_text_box( TextBox& text_box, float& panel_y)
{
    const float height = text_box_height + 2 * panel_spacer;
    Rectangle rect = { panel_x, panel_y, panel_width - 2 * panel_spacer, height };
    GuiGroupBox( rect, text_box.label.c_str());
    rect.x += panel_spacer;
    rect.y += panel_spacer;
    rect.width -= 2 * panel_spacer;
    rect.height -= 2 * panel_spacer;
    panel_y += height + panel_spacer;

    if (GuiTextBox( rect, text_box.text.data(), text_box.text.size(), text_box.edit_mode))
        text_box.edit_mode = !text_box.edit_mode;
}

namespace gui {

struct Human
{
    static const int idx = 0;
};

struct Algo
{
    Spinner depth = Spinner( "depth", 1, 1, 10 );
    Menu ttt_eval_menu = Menu {"score heuristic", {"trivial estimate", "simple estimate"}, 1};
    Menu uttt_eval_menu = Menu {"score heuristic", {"simple estimate"}};
};

struct Negamax : public Algo
{
    static const int idx = 1;
  //  future< pair< ttt::Move, unique_ptr< ::Algorithm< ttt::Move > > > > ttt_get_move;
  //  future< pair< uttt::Move, unique_ptr< ::Algorithm< uttt::Move > > > > uttt_get_move;
  
    Menu reorder_menu = Menu { "reorder moves", {"shuffle", "reorder by score"} };
};

struct Minimax : public Algo
{
    static const int idx = 2;

    Menu recursion_menu = Menu { "recursion", {"max depth", "max vertices"}};
    Menu choose_menu = Menu {"choose", {"best", "epsilon bucket"}};
};

struct Montecarlo
{
    static const int idx = 3;
    Menu choose_menu = Menu {"choose", {"best"}};
    Spinner simulations = Spinner( "simulations", 1000, 100, 100000 );
    TextBox exploration_factor = TextBox( "exploration factor", "0.50" );
};

struct Player
{
    Player( string const& name, ::Player player ) : name( name ), player( player ) {}
    string name;
    ::Player player;
    Menu algo_menu = Menu { "algorithm", {"human", "negamax", "minimax", "montecarlo"} };

    Negamax negamax;
    Minimax minimax;
    Montecarlo montecarlo;
};

struct Game 
{
    enum Type { TicTacToe, UltimateTicTacToe };
    Player player1 = Player( "player1", ::player1 );
    Player player2 = Player( "player2", ::player2 );
    Player* next_move = &player1;
    ::Player winner = not_set;
};

} // namespace gui {

Menu game_menu = Menu { "game", {"tic tac toe", "ultimate tic tac toe"} };
gui::Game games[2];
enum UIState { ConfigureGame, PlayGame };
UIState ui_state = ConfigureGame;

optional< uint8_t > handle_board_event( 
    ::Player const* board, vector< uint8_t > const& valid_moves, ::Player player, Convert convert, int number_of_cells )  
{
    auto cell_indices = get_cell_indices( number_of_cells );
    if (!cell_indices)
        return {};
    const float cell_size = board_width / number_of_cells;
    const uint8_t move = convert(*cell_indices);

    if (   ui_state == PlayGame 
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

void show_side_panel()
{
    float panel_y = panel_spacer;
    gui::Game& game = games[game_menu.selected];
    Player& player = *game.next_move;
    Menu* eval_menu = 0;
    DropDownMenu dropdown_menu;

    if (ui_state == ConfigureGame)
    {
        dropdown_menu.add( game_menu, panel_y );
        if (show_button( "toggle player", game.next_move->name, panel_y ))
            game.next_move = (game.next_move == &game.player1) 
                ? &game.player2 : &game.player1; 
                
        dropdown_menu.add( player.algo_menu, panel_y);
        if (player.algo_menu.selected == gui::Minimax::idx)
        {
            if (game_menu.selected == gui::Game::TicTacToe)
                eval_menu = &player.minimax.ttt_eval_menu;
            else if (game_menu.selected == gui::Game::UltimateTicTacToe)
                eval_menu = &player.minimax.uttt_eval_menu;
            assert (eval_menu);
            dropdown_menu.add( *eval_menu, panel_y);
            dropdown_menu.add( player.minimax.recursion_menu, panel_y);
            dropdown_menu.add( player.minimax.choose_menu, panel_y);
            show_spinner( player.minimax.depth, panel_y);
        }
        else if (player.algo_menu.selected == gui::Negamax::idx)
        {
            if (game_menu.selected == gui::Game::TicTacToe)
                eval_menu = &player.negamax.ttt_eval_menu;
            else if (game_menu.selected == gui::Game::UltimateTicTacToe)
                eval_menu = &player.negamax.uttt_eval_menu;
            assert (eval_menu);
            dropdown_menu.add( *eval_menu, panel_y);
            dropdown_menu.add( player.negamax.reorder_menu, panel_y);
            show_spinner( player.negamax.depth, panel_y);
        }
        else if (player.algo_menu.selected == gui::Montecarlo::idx)
        {
            dropdown_menu.add( player.montecarlo.choose_menu, panel_y);
            show_spinner( player.montecarlo.simulations, panel_y);
            show_text_box( player.montecarlo.exploration_factor, panel_y);
        }

        if (show_button( "start game", panel_y))
        {
            ui_state = PlayGame;
            game.winner = not_set;
        }
    }
    else if (ui_state == PlayGame)
    {
        show_label( "next move", game.next_move->name, panel_y );

        const char* button_text = 0;
        string status_text;
        if (game.winner == not_set)
        { 
            button_text = "abort game";
            status_text = game.next_move->algo_menu.items[game.next_move->algo_menu.selected] + " thinking...";
        }
        else
        {
            button_text = "back";
            status_text = (game.winner == player1 
                ? game.player1.name 
                : game.player2.name) + " wins!";
        }

        show_label( "status", status_text.c_str(), panel_y );
        
        if (show_button( button_text, panel_y))
            ui_state = ConfigureGame;
    }
}

void show_board()
{
    gui::Game& game = games[game_menu.selected];
    if (game_menu.selected == gui::Game::TicTacToe)
    {
        using namespace tic_tac_toe;

        draw_board( rule.board, last_move );

        if (ui_state == PlayGame)
        {
            optional< Move > move;
            if (game.next_move->algo_menu.selected == gui::Human::idx && game.winner == not_set)
                move = handle_board_event( 
                    rule.board, valid_moves, game.next_move->player, cell_indices_to_move, n );

            if (move)
            {
                last_move = move;
                rule.apply_move( *move, game.next_move->player );
                valid_moves = rule.generate_moves();
                game.winner = rule.get_winner();
                game.next_move = game.next_move == &game.player1 
                    ? &game.player2 : &game.player1;
            }
        }
        else if (ui_state == ConfigureGame)
        {
            optional< Move > move = handle_board_event( 
                rule.board, valid_moves, game.next_move->player, cell_indices_to_move, n );
            last_move.reset();
            if (move)
            {
                if (rule.board[*move] != not_set)
                    rule.undo_move( *move, game.next_move->player );
                else 
                    rule.apply_move( *move, game.next_move->player );
                
                valid_moves = rule.generate_moves();
                game.winner = rule.get_winner();
            }
        }

    }
    else if (game_menu.selected == gui::Game::UltimateTicTacToe)
    {
        using namespace meta_tic_tac_toe;

        draw_board( rule, last_move );

        if (ui_state == PlayGame)
        {
            // indicate valid outer board(s)
            for (Move move : valid_moves)
            {
                div_t d = div( move, n * n);
                div_t ij = div( d.quot, n );
                draw_box( ij.quot, ij.rem, RED, outer_cell_size, 0, 0, 2 );
            }

            optional< Move > move;
            if (game.next_move->algo_menu.selected == gui::Human::idx && game.winner == not_set)
                move = handle_board_event( 
                    rule.board.data(), valid_moves, game.next_move->player, cell_indices_to_move, n * n );
            if (move)
            {
                last_move = move;
                rule.apply_move( *move, game.next_move->player );
                valid_moves = rule.generate_moves();
                game.winner = rule.get_winner();
                game.next_move = game.next_move == &game.player1
                    ? &game.player2 : &game.player1;
            }
        }
        else if (ui_state == ConfigureGame)
        {
            optional< Move > move = handle_board_event( 
                rule.board.data(), valid_moves, game.next_move->player, cell_indices_to_move, n * n );
            last_move.reset();
            if (move)
            {
                if (rule.board[*move] != not_set)
                    rule.undo_move( *move, game.next_move->player );
                else 
                    rule.apply_move( *move, game.next_move->player );
                
                valid_moves = rule.generate_moves();
                game.winner = rule.get_winner();
            }
        }
    }
    else
        assert (false);
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
        InitWindow(window_width, window_height, "(ultimate) tic tac toe");
        SetTargetFPS(10);
    }

    ~RaylibInit()
    {
        CloseWindow();
    }
};

void show()
{
    RaylibInit raylib_init;

    while (!WindowShouldClose())
    {
        RaylibRender raylib_render;

        show_side_panel();
        show_board();
    }
}

} // namespace gui {
