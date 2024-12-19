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

void Game::show_graph( MouseEvent& mouse_event)
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
                    current_mouse_pos.x, current_mouse_pos.y );
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

void Game::show_side_panel()
{
    DropDownMenu dropdown_menu;

    panel_y = panel_spacer;

    if (ui_state == ConfigureGame)
    {
        show_label( "mode", "configure" );
        dropdown_menu.add( game_menu );
        if (show_button( "toggle player", current_player->name ))
            std::swap( current_player, opponent );
                
        dropdown_menu.add( current_player->algo_menu );
        current_player->get_algo().show_side_panel( dropdown_menu);

        dropdown_menu.add( play_mode_menu );

        if (show_button( "start game" ))
        {
            start_game();
            on_hold = (   play_mode_menu.selected == SingleStepPlayIdx
                        && current_player->algo_menu.selected != Player::HumanIdx);
            show_menu.selected = BoardIdx;
            ui_state = PlayGame;
        }
    }
    else if (ui_state == PlayGame)
    {
        const bool game_finished = game_has_finished();
        const bool waiting_for_move = !on_hold && !game_finished;
        if (waiting_for_move)
            process_move();

        const char* button_text = 0;
        static std::string status_text;
        if (winner == current_player->player)
        {
            status_text = current_player->name + " wins";
            button_text = "back";
        }
        else if (winner == opponent->player)
        {
            status_text = opponent->name + " wins";
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
        
        current_player->show_game_info( waiting_for_move );
        opponent->show_game_info( false );

        show_label( "status", status_text.c_str());

        if (play_mode_menu.selected == SingleStepPlayIdx)
        {
            dropdown_menu.add( show_menu );

            if (show_menu.selected == TreeIdx)
            {
                auto& algo = opponent->get_algo();
                bool tree_controls_changed = algo.show_tree_controls( dropdown_menu);
                
                if (tree_controls_changed)
                {
                    std::cout << "tree controls changed" << std::endl;
                    algo.reset_texture();
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
            if (current_player->player != ::player1)
                std::swap( current_player, opponent );

            mouse_event = MouseEvent();

            ui_state = ConfigureGame;
        }
    }
}

void Game::show_main_panel()
{
    if (   ui_state == ConfigureGame
        || opponent->algo_menu.selected == gui::Player::HumanIdx 
        || show_menu.selected == gui::Game::BoardIdx)
        show_board( ui_state );
    else if (show_menu.selected == gui::Game::TreeIdx)
        show_graph( mouse_event );
}

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
    auto algo = dynamic_cast< interactive::Algorithm< tic_tac_toe::Move >* >( 
        current_player->get_algo().get_algorithm());
    if (!algo)
        throw runtime_error( "invalid interactive algorithm");

    algo->set_move( move );
    winner = rule->get_winner();
}

void TicTacToe::process_config_move( tic_tac_toe::Move const& move )
{
    if (dynamic_cast< tic_tac_toe::Rule* >( rule.get())->board[move] != not_set)
        rule->undo_move( move, current_player->player );
    else 
        rule->apply_move( move, current_player->player );
}

void TicTacToe::reset()
{
    rule.reset( new tic_tac_toe::DeepRule());
    GameGenerics::reset();
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
    if (ui_state == PlayGame)
        // indicate valid outer board(s)
        for (meta_tic_tac_toe::Move move : valid_moves)
        {
            div_t d = div( move, meta_tic_tac_toe::n * meta_tic_tac_toe::n);
            div_t ij = div( d.quot, meta_tic_tac_toe::n );

            draw_box( ij.quot, ij.rem, RED, board_width / 3, 0, 0, 2 );
        }

    return handle_board_event( 
        dynamic_cast< meta_tic_tac_toe::Rule* >( rule.get())->board.data(), 
        valid_moves, current_player->player, cell_indices_to_move, meta_tic_tac_toe::n * meta_tic_tac_toe::n);
}

void MetaTicTacToe::process_play_move( meta_tic_tac_toe::Move const& move )
{
    auto algo = dynamic_cast< interactive::Algorithm< meta_tic_tac_toe::Move >* >( 
        current_player->get_algo().get_algorithm());
    if (!algo)
        throw runtime_error( "invalid interactive algorithm");
 
    algo->set_move( move );    
    winner = rule->get_winner();
}

void MetaTicTacToe::process_config_move( meta_tic_tac_toe::Move const& move )
{
    if (dynamic_cast< meta_tic_tac_toe::Rule* >( rule.get())->board.data()[move] != not_set)
        rule->undo_move( move, current_player->player );
    else 
        rule->apply_move( move, current_player->player );
}

void MetaTicTacToe::reset()
{
    rule.reset( new meta_tic_tac_toe::Rule());
    GameGenerics::reset();
}

} // namespace gui {
