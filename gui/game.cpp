#include "game.h"

using namespace std;

namespace gui {

UIState ui_state = ConfigureGame;

GVC_t* gv_gvc = nullptr;
MouseEvent mouse_event;

Menu game_menu = Menu { "game", {"tic tac toe", "ultimate tic tac toe"} };
unique_ptr< Game > games[2];

typedef uint8_t (*Convert)( pair< int, int > const& cell_indices );

void draw_player(
    ::Player player, int i, int j, Color color, float cell_size, float pos_x = 0, float pos_y = 0)
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

Game::~Game() {}

TicTacToe::TicTacToe() : GameGenerics< tic_tac_toe::Move >( 
    new TicTacToePlayer( "player x", ::player1 ), 
    new TicTacToePlayer( "player o", ::player2 ),
    new tic_tac_toe::DeepRule()) {}

void TicTacToe::draw_board( float board_width) 
{
    ::Player const* const board = dynamic_cast< tic_tac_toe::Rule* >( rule.get())->board;
    const float cell_size = board_width / 3;
    for (int i = 0; i < tic_tac_toe::n; i++)
        for (int j = 0; j < tic_tac_toe::n; j++)
        {
            const int idx = i * tic_tac_toe::n + j;
            const ::Player player = board[idx];
            const Color player_color = last_move == idx ? RED : BLACK;
            draw_box( i, j, BLACK, cell_size);
            draw_player(player, i, j, player_color, cell_size);
        }
}

u_int8_t TicTacToe::cell_indices_to_move( pair< int, int > const& cell_indices )
{
    return cell_indices.first * tic_tac_toe::n + cell_indices.second;
}

optional< tic_tac_toe::Move > TicTacToe::get_move() 
{
    return handle_board_event( 
        dynamic_cast< tic_tac_toe::Rule* >( rule.get())->board, 
        valid_moves, current_player->player, cell_indices_to_move, tic_tac_toe::n );
}

void TicTacToe::process_play_move( tic_tac_toe::Move const& move )
{
    if (dynamic_cast< tic_tac_toe::Rule* >( rule.get())->board[move] != not_set)
        rule->undo_move( move, current_player->player );
    else 
        rule->apply_move( move, current_player->player );
    
    winner = rule->get_winner();
}

void TicTacToe::process_config_move( tic_tac_toe::Move const& move )
{
    auto algo = dynamic_cast< interactive::Algorithm< tic_tac_toe::Move >* >( 
        current_player->get_algo().get_algorithm());
    if (!algo)
        throw runtime_error( "invalid interactive algorithm");
    algo->set_move( move );
}

void TicTacToe::reset()
{
    GameGenerics::reset();
    rule.reset( new tic_tac_toe::DeepRule());
}

MetaTicTacToe::MetaTicTacToe() : GameGenerics< meta_tic_tac_toe::Move >( 
    new MetaTicTacToePlayer( "player x", ::player1), 
    new MetaTicTacToePlayer( "player o", ::player2 ), 
    new meta_tic_tac_toe::Rule()) {}

void MetaTicTacToe::draw_board( float board_width )
{
    meta_tic_tac_toe::Rule const& r = dynamic_cast< meta_tic_tac_toe::Rule& >( *rule );
    const float outer_cell_size = board_width / 3;
    const float inner_cell_size = outer_cell_size / 3;
    int idx = 0;
    for (int i = 0; i < meta_tic_tac_toe::n; i++)
        for (int j = 0; j < meta_tic_tac_toe::n; j++)
        {
            const bool terminal = r.terminals[i * meta_tic_tac_toe::n + j];

            draw_box( i, j, BLACK, outer_cell_size, 0, 0, 2 );
            const int pos_x = j * outer_cell_size;
            const int pos_y = i * outer_cell_size;
            for (int i2 = 0; i2 < meta_tic_tac_toe::n; i2++)
                for (int j2 = 0; j2 < meta_tic_tac_toe::n; j2++)
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
                draw_player( r.meta_board[i * meta_tic_tac_toe::n + j], i, j, BLACK, outer_cell_size, 0, 0);
        }
}

u_int8_t MetaTicTacToe::cell_indices_to_move( pair< int, int > const& cell_indices )
{
    const div_t x_coord = div( cell_indices.second, meta_tic_tac_toe::n );
    const div_t y_coord = div( cell_indices.first, meta_tic_tac_toe::n );
    return y_coord.quot * meta_tic_tac_toe::n * meta_tic_tac_toe::n * meta_tic_tac_toe::n + y_coord.rem * meta_tic_tac_toe::n 
            + x_coord.quot * meta_tic_tac_toe::n * meta_tic_tac_toe::n + x_coord.rem;
}

optional< meta_tic_tac_toe::Move > MetaTicTacToe::get_move() 
{
    return handle_board_event( 
        dynamic_cast< meta_tic_tac_toe::Rule* >( rule.get())->board.data(), 
        valid_moves, current_player->player, cell_indices_to_move, meta_tic_tac_toe::n * meta_tic_tac_toe::n);
}

void MetaTicTacToe::process_play_move( meta_tic_tac_toe::Move const& move )
{
    if (dynamic_cast< meta_tic_tac_toe::Rule* >( rule.get())->board.data()[move] != not_set)
        rule->undo_move( move, current_player->player );
    else 
        rule->apply_move( move, current_player->player );
    
    winner = rule->get_winner();
}

void MetaTicTacToe::process_config_move( meta_tic_tac_toe::Move const& move )
{
    // indicate valid outer board(s)
    for (meta_tic_tac_toe::Move move : valid_moves)
    {
        div_t d = div( move, meta_tic_tac_toe::n * meta_tic_tac_toe::n);
        div_t ij = div( d.quot, meta_tic_tac_toe::n );

        draw_box( ij.quot, ij.rem, RED, board_width / 3, 0, 0, 2 );
    }

    if (move)
    {
        auto algo = dynamic_cast< interactive::Algorithm< meta_tic_tac_toe::Move >* >( 
            current_player->get_algo().get_algorithm());
        if (!algo)
            throw runtime_error( "invalid interactive algorithm");
        algo->set_move( move );
    }
}
void MetaTicTacToe::reset()
{
    GameGenerics::reset();
    rule.reset( new meta_tic_tac_toe::Rule());
}

} // namespace gui {
