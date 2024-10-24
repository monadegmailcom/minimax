#include "meta_tic_tac_toe.h"
#include "game.h"

// for some reasons this has to be defined
#define RAYGUI_IMPLEMENTATION
// patch missing declaration because RAYGUI_STANDALONE is not defined
static float TextToFloat(const char *text);
#include "raygui.h"

#include <iostream>
#include <string>
#include <cassert>
#include <thread>
#include <future>

// patch missing definition because RAYGUI_STANDALONE is not defined
static float TextToFloat(const char *text)
{
    float value = 0.0f;
    float sign = 1.0f;

    if ((text[0] == '+') || (text[0] == '-'))
    {
        if (text[0] == '-') sign = -1.0f;
        text++;
    }

    int i = 0;
    for (; ((text[i] >= '0') && (text[i] <= '9')); i++) value = value*10.0f + (float)(text[i] - '0');

    if (text[i++] != '.') value *= sign;
    else
    {
        float divisor = 10.0f;
        for (; ((text[i] >= '0') && (text[i] <= '9')); i++)
        {
            value += ((float)(text[i] - '0'))/divisor;
            divisor = divisor*10.0f;
        }
    }

    return value;
}

using namespace std;
using namespace placeholders;

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

template< typename MoveT >
struct GameGenerics
{
    GameGenerics( GenericRule< MoveT >& rule, GenericRule< MoveT >& initial_rule ) : 
        rule( rule ), initial_rule( initial_rule )
    {
        reset();
    }

    void reset()
    {
        rule.copy_from( initial_rule );
        last_move.reset();
        valid_moves = rule.generate_moves();
    }

    GenericRule< MoveT >& rule;
    GenericRule< MoveT >& initial_rule;
    optional< MoveT > last_move;
    vector< MoveT > valid_moves;
    unique_ptr< ::Algorithm< MoveT > > player1_algo;
    unique_ptr< ::Algorithm< MoveT > > player2_algo;
};

namespace tic_tac_toe {

const float cell_size = board_width / 3;
DeepRule rule;
DeepRule initial_rule;
GameGenerics< ttt::Move > game_generics { rule, initial_rule };

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

GameGenerics< uttt::Move > game_generics { rule, initial_rule };

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

void show_float_value_box( ValueBoxFloat& value_box, float& panel_y)
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

namespace gui {

struct Human
{};

struct Algo
{
    Spinner depth = Spinner( "depth", 7, 1, 15 );
    ValueBoxFloat uttt_score_weight = ValueBoxFloat( "score weight", "9.0" );

    enum EvalIdx { SimpleEstimateIdx, TrivialEstimateIdx };
    Menu ttt_eval_menu = Menu {"score heuristic", {"simple estimate", "trivial estimate"}, 0};
    Menu uttt_eval_menu = Menu {"score heuristic", {"simple estimate"}};
};

struct Negamax : public Algo
{
    enum ReorderIdx { ShuffleIdx, ReorderByScoreIdx };
    Menu reorder_menu = Menu { "reorder moves", {"shuffle", "reorder by score"}, 1 };
};

struct Minimax : public Algo
{
    Spinner max_vertices = Spinner( "max vertices", 280000, 1, 1000000 );

    enum RecursionIdx { MaxDepthIdx, MaxVerticesIdx };
    Menu recursion_menu = Menu { "recursion", {"max depth", "max vertices"}};

    enum ChooseIdx { BestIdx, EpsilonBucketIdx };
    Menu choose_menu = Menu {"choose", {"best", "epsilon bucket"}};
    ValueBoxFloat bucket_width = ValueBoxFloat( "bucket width", "1.00" );
};

struct Montecarlo
{
    enum ChooseIdx { BestIdx };
    Menu choose_menu = Menu {"choose", {"best"}};
    Spinner simulations = Spinner( "simulations", 80000, 1, 1000000 );
    ValueBoxFloat exploration_factor = ValueBoxFloat( "exploration factor", "0.40" );
};

struct Player
{
    Player( string const& name, ::Player player ) : name( name ), player( player ) {}
    string name;
    ::Player player;
    enum AlgoIdx { HumanIdx, NegamaxIdx, MinimaxIdx, MontecarloIdx };
    Menu algo_menu = Menu { "algorithm", {"human", "negamax", "minimax", "montecarlo"} };

    Negamax negamax;
    Minimax minimax;
    Montecarlo montecarlo;
};

struct Game 
{
    Player player1 = Player( "player1", ::player1 );
    Player player2 = Player( "player2", ::player2 );
    Player* current_player = &player1;
    Player* opponent = &player2;
    ::Player winner = not_set;
};

} // namespace gui {

enum GameIdx { TicTacToeIdx, UltimateTicTacToeIdx };
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

function< double (GenericRule< ttt::Move >&, ::Player) > make_ttt_eval_function( int idx )
{
    if (idx == gui::Algo::TrivialEstimateIdx)
        return [](auto& rule, auto) 
            { return ttt::trivial_estimate::eval( dynamic_cast< ttt::Rule const& >( rule ));};
    else if (idx == gui::Algo::SimpleEstimateIdx)
        return []( auto& rule, auto) 
            { return ttt::simple_estimate::eval( dynamic_cast< ttt::Rule const& >( rule ));};
    else
        throw runtime_error( "invalid ttt evaluation function");
}

function< double (GenericRule< uttt::Move >&, ::Player) > make_uttt_eval_function( int idx, double weight )
{
    if (idx == gui::Minimax::SimpleEstimateIdx)
        return [weight]
            (GenericRule< uttt::Move >& rule, ::Player) 
            { return uttt::simple_estimate::eval( dynamic_cast< uttt::Rule& >( rule ), weight );};
    else
        throw runtime_error( "invalid uttt evaluation function");
}

template< typename MoveT >
ReOrder< MoveT > make_reorder_function( int idx, function< double (GenericRule< MoveT >&, ::Player) > eval )
{
    if (idx == gui::Negamax::ShuffleIdx)
        return [shuffle = make_shared< Shuffle< MoveT > >()]
            (auto& rule, auto player, auto begin, auto end) 
            { (*shuffle)(rule, player, begin, end);};
    else if (idx == gui::Negamax::ReorderByScoreIdx)
        return [rbs = make_shared< ReorderByScore< MoveT > >( eval )]
            (auto& rule, ::Player player, auto begin, auto end )
            { (*rbs)( rule, player, begin, end ); };
    else
        throw runtime_error( "invalid reorder function");
}

template< typename MoveT >
Recursion< ttt::Move >* make_recursion_function( gui::Player const& player )
{
    if (player.minimax.recursion_menu.selected == gui::Minimax::MaxDepthIdx)
        return new MaxDepth< MoveT >( player.minimax.depth.value );
    else if (player.minimax.recursion_menu.selected == gui::Minimax::MaxVerticesIdx)
        return new MaxVertices< MoveT >( player.minimax.max_vertices.value );
    else
        throw runtime_error( "invalid recursion function");
}

template< typename MoveT >
function< MoveT const& (VertexList< MoveT > const&) > make_choose_move_function( gui::Player const& player )
{
    if (player.minimax.choose_menu.selected == gui::Minimax::BestIdx)
        return bind( &ChooseFirst< MoveT >::operator(), ChooseFirst< MoveT >(), _1 );
    else if (player.minimax.choose_menu.selected == gui::Minimax::EpsilonBucketIdx)
        return [cm = make_shared< ChooseMove< MoveT > >( player.minimax.bucket_width.value)]
            (VertexList< MoveT > const& vertices)
            { return (*cm)( vertices ); };
    else
        throw runtime_error( "invalid choose move function");
}

template< typename MoveT >
montecarlo::ChooseMove< MoveT >* make_mc_choose_move_function( int idx )
{
    if (idx == gui::Montecarlo::BestIdx)
        return new montecarlo::ChooseBest< MoveT >();
    else
        throw runtime_error( "invalid montecarlo choose move function");
}

// return true if the game is finished
template< typename MoveT >
bool process_move( GameGenerics< MoveT >& game_generics )
{
    gui::Game& game = games[game_menu.selected];
    if (game.winner != not_set || game_generics.valid_moves.empty())
        return true;

    auto current_player_algo = game_generics.player1_algo.get();
    auto opponent_algo = game_generics.player2_algo.get();
    if (game.current_player == &game.player2)
        swap( current_player_algo, opponent_algo);

    if (!current_player_algo || !opponent_algo)
        throw runtime_error( "algo not set (process move)");

    optional< MoveT > move = current_player_algo->get_move();
    if (move)
    {
        game_generics.last_move = move;
        game_generics.rule.apply_move( *move, game.current_player->player );
        game_generics.valid_moves = game_generics.rule.generate_moves();

        game.winner = game_generics.rule.get_winner();
        swap( game.current_player, game.opponent );

        opponent_algo->opponent_move( *move );
    }

    return false;
}

template <typename MoveT>
unique_ptr< ::Algorithm< MoveT > >& get_algo( 
    GameGenerics< MoveT >& game_generics, gui::Player& player )
{
    gui::Game& game = games[game_menu.selected];
    if (&player == &game.player1)
        return game_generics.player1_algo;
    else if (&player == &game.player2)
        return game_generics.player2_algo;
    else
        throw runtime_error( "invalid player (get_algo)");
}

void start_game( gui::Player& player )
{
    if (player.algo_menu.selected == gui::Player::NegamaxIdx)
    {
        if (game_menu.selected == TicTacToeIdx)
        {
            auto eval = make_ttt_eval_function( player.negamax.ttt_eval_menu.selected );
            auto reorder = make_reorder_function< ttt::Move >(
                player.negamax.reorder_menu.selected, eval );

            get_algo( ttt::game_generics, player ) = make_unique< NegamaxAlgorithm< ttt::Move > >(
                ttt::rule, player.player, player.negamax.depth.value, 
                reorder, eval, false );
        }
        else if (game_menu.selected == UltimateTicTacToeIdx)
        {
            auto eval = make_uttt_eval_function( 
                player.negamax.uttt_eval_menu.selected, 
                player.negamax.uttt_score_weight.value );
            auto reorder = make_reorder_function< uttt::Move >(
                player.negamax.reorder_menu.selected, eval );

            get_algo( uttt::game_generics, player ) = make_unique< NegamaxAlgorithm< uttt::Move > >(
                uttt::rule, player.player, player.negamax.depth.value, 
                    reorder, eval, false);
        }
        else
            throw runtime_error( "invalid game");
    }
    else if (player.algo_menu.selected == gui::Player::MinimaxIdx)
    {
        if (game_menu.selected == TicTacToeIdx)
        {
            auto eval = make_ttt_eval_function( player.minimax.ttt_eval_menu.selected );
            auto recursion = make_recursion_function< ttt::Move >( player );
            auto choose_move = make_choose_move_function< ttt::Move >( player );   

            get_algo( ttt::game_generics, player ) = make_unique< MinimaxAlgorithm< ttt::Move > >(
                ttt::rule, player.player, eval, recursion, choose_move, false );
        }
        else if (game_menu.selected == UltimateTicTacToeIdx)
        {
            auto eval = make_uttt_eval_function( 
                player.minimax.uttt_eval_menu.selected, 
                player.minimax.uttt_score_weight.value );
            auto recursion = make_recursion_function< uttt::Move >( player );
            auto choose_move = make_choose_move_function< uttt::Move >( player );   

            get_algo( uttt::game_generics, player ) = make_unique< MinimaxAlgorithm< uttt::Move > >(
                uttt::rule, player.player, eval, recursion, choose_move, false );
        }
        else
            throw runtime_error( "invalid game");
    }
    else if (player.algo_menu.selected == gui::Player::MontecarloIdx)
    {
        if (game_menu.selected == TicTacToeIdx)
        {
            auto choose_move = make_mc_choose_move_function< ttt::Move >(
                player.montecarlo.choose_menu.selected );

            get_algo( ttt::game_generics, player ) = make_unique< montecarlo::Algorithm< ttt::Move > >(
                ttt::rule, player.player, choose_move, player.montecarlo.simulations.value, 
                player.montecarlo.exploration_factor.value, false );
        }
        else if (game_menu.selected == UltimateTicTacToeIdx)
        {
            auto choose_move = make_mc_choose_move_function< uttt::Move >(
                player.montecarlo.choose_menu.selected );

            get_algo( uttt::game_generics, player ) = make_unique< montecarlo::Algorithm< uttt::Move > >(
                uttt::rule, player.player, choose_move, player.montecarlo.simulations.value, 
                player.montecarlo.exploration_factor.value, false );
        }
        else
            throw runtime_error( "invalid game");
    }  
    else if (player.algo_menu.selected == gui::Player::HumanIdx)
    {
        if (game_menu.selected == TicTacToeIdx)
            get_algo( ttt::game_generics, player ) = make_unique< interactive::Algorithm< ttt::Move > >( 
                ttt::rule, player.player );
        else if (game_menu.selected == UltimateTicTacToeIdx)
            get_algo( uttt::game_generics, player ) = make_unique< interactive::Algorithm< uttt::Move > >( 
                uttt::rule, player.player );
        else
            throw runtime_error( "invalid game");
    }
    else
        throw runtime_error( "invalid algorithm");
}

template< typename MoveT >
void stop_algo( unique_ptr< ::Algorithm< MoveT > >& algo)
{
    if (algo)
    {
        // request algo to stop and wait until it finishes
        algo->stop();
        // release algo resource
        algo.reset();
    }
}

template< typename MoveT >
void stop_game( GameGenerics< MoveT >& game_generics )
{
    stop_algo( game_generics.player1_algo );
    stop_algo( game_generics.player2_algo );

    game_generics.reset();
    games[game_menu.selected].winner = not_set;
}

void show_side_panel()
{
    float panel_y = panel_spacer;
    gui::Game& game = games[game_menu.selected];
    DropDownMenu dropdown_menu;

    if (ui_state == ConfigureGame)
    {
        show_label( "mode", "configure", panel_y );
        dropdown_menu.add( game_menu, panel_y );
        if (show_button( "toggle player", game.current_player->name, panel_y ))
            swap( game.current_player, game.opponent );
                
        dropdown_menu.add( game.current_player->algo_menu, panel_y);

        if (game.current_player->algo_menu.selected == gui::Player::MinimaxIdx)
        {
            if (game_menu.selected == TicTacToeIdx)
                dropdown_menu.add( game.current_player->minimax.ttt_eval_menu, panel_y );
            else if (game_menu.selected == UltimateTicTacToeIdx)
                dropdown_menu.add( game.current_player->minimax.uttt_eval_menu, panel_y );
            dropdown_menu.add( game.current_player->minimax.recursion_menu, panel_y);
            dropdown_menu.add( game.current_player->minimax.choose_menu, panel_y);
            if (game_menu.selected == UltimateTicTacToeIdx)
                show_float_value_box( game.current_player->minimax.uttt_score_weight, panel_y);
            if (game.current_player->minimax.recursion_menu.selected == gui::Minimax::MaxVerticesIdx)
                show_spinner( game.current_player->minimax.max_vertices, panel_y);
            else if (game.current_player->minimax.recursion_menu.selected == gui::Minimax::MaxDepthIdx)
                show_spinner( game.current_player->minimax.depth, panel_y);
            if (game.current_player->minimax.choose_menu.selected == gui::Minimax::EpsilonBucketIdx)
                show_float_value_box( game.current_player->minimax.bucket_width, panel_y);
        }
        else if (game.current_player->algo_menu.selected == gui::Player::NegamaxIdx)
        {
            if (game_menu.selected == TicTacToeIdx)
                dropdown_menu.add( game.current_player->negamax.ttt_eval_menu, panel_y);
            else if (game_menu.selected == UltimateTicTacToeIdx)
                dropdown_menu.add( game.current_player->negamax.uttt_eval_menu, panel_y);
            dropdown_menu.add( game.current_player->negamax.reorder_menu, panel_y);
            if (game_menu.selected == UltimateTicTacToeIdx)
                show_float_value_box( game.current_player->minimax.uttt_score_weight, panel_y);
            show_spinner( game.current_player->negamax.depth, panel_y);
        }
        else if (game.current_player->algo_menu.selected == gui::Player::MontecarloIdx)
        {
            dropdown_menu.add( game.current_player->montecarlo.choose_menu, panel_y);
            show_spinner( game.current_player->montecarlo.simulations, panel_y);
            show_float_value_box( game.current_player->montecarlo.exploration_factor, panel_y);
        }

        if (show_button( "start game", panel_y))
        {
            start_game( game.player1 );
            start_game( game.player2 );

            ui_state = PlayGame;
        }
    }
    else if (ui_state == PlayGame)
    {
        bool game_finished = false;

        if (game_menu.selected == TicTacToeIdx)
            game_finished = process_move< ttt::Move >( ttt::game_generics );
        else if (game_menu.selected == UltimateTicTacToeIdx)
            game_finished = process_move< uttt::Move >( uttt::game_generics );
        else
            throw runtime_error( "invalid game (PlayGame))");

        const char* button_text = 0;
        string status_text;
        if (game.winner == player1)
        {
            status_text = game.player1.name + " wins";
            button_text = "back";
        }
        else if (game.winner == player2)
        {
            status_text = game.player2.name + " wins";
            button_text = "back";
        }
        else if (game_finished)
        {
            status_text = "draw";
            button_text = "back";
        }
        else if (game.current_player == &game.player1)
        {
            status_text = game.player1.name + "'s turn";
            button_text = "abort game";
        }
        else if (game.current_player == &game.player2)
        {
            status_text = game.player2.name + "'s turn";
            button_text = "abort game";
        }

        show_label( "mode", "play", panel_y );
        show_label( 
            game.player1.name, 
            game.player1.algo_menu.items[game.player1.algo_menu.selected], panel_y );
        show_label( 
            game.player2.name, 
            game.player2.algo_menu.items[game.player2.algo_menu.selected], panel_y );
        show_label( "status", status_text.c_str(), panel_y );
        if (show_button( button_text, panel_y))
        {
            if (game_menu.selected == TicTacToeIdx)
                stop_game( ttt::game_generics );
            else if (game_menu.selected == UltimateTicTacToeIdx)
                stop_game( uttt::game_generics );
            else
                throw runtime_error( "invalid game (show_button)");

            if (game.current_player != &game.player1)
                swap( game.current_player, game.opponent );

            ui_state = ConfigureGame;
        }
    }
}

void show_board()
{
    gui::Game& game = games[game_menu.selected];
    if (game_menu.selected == TicTacToeIdx)
    {
        using namespace tic_tac_toe;

        draw_board( rule.board, game_generics.last_move );

        if (   ui_state == PlayGame 
            && game.winner == not_set
            && game.current_player->algo_menu.selected == gui::Player::HumanIdx)
        {
            optional< Move > move = handle_board_event( 
                rule.board, game_generics.valid_moves, game.current_player->player, 
                cell_indices_to_move, n );
            if (move)
            {
                auto algo = dynamic_cast< interactive::Algorithm< ttt::Move >* >( 
                    get_algo( ttt::game_generics, *game.current_player).get());
                if (!algo)
                    throw runtime_error( "invalid interactive ttt algorithm");
                algo->move_promise.set_value( *move );
            }
        }
        else if (ui_state == ConfigureGame)
        {
            optional< Move > move = handle_board_event( 
                rule.board, game_generics.valid_moves, game.current_player->player, 
                cell_indices_to_move, n );
            
            if (move)
            {
                if (rule.board[*move] != not_set)
                    rule.undo_move( *move, game.current_player->player );
                else 
                    rule.apply_move( *move, game.current_player->player );
                
                game.winner = rule.get_winner();
            }
        }
    }
    else if (game_menu.selected == UltimateTicTacToeIdx)
    {
        using namespace meta_tic_tac_toe;

        draw_board( rule, game_generics.last_move );

        if (   ui_state == PlayGame 
            && game.winner == not_set
            && game.current_player->algo_menu.selected == gui::Player::HumanIdx)
        {
            // indicate valid outer board(s)
            for (Move move : game_generics.valid_moves)
            {
                div_t d = div( move, n * n);
                div_t ij = div( d.quot, n );
                draw_box( ij.quot, ij.rem, RED, outer_cell_size, 0, 0, 2 );
            }

            optional< Move > move = handle_board_event( 
                rule.board.data(), game_generics.valid_moves, game.current_player->player, 
                cell_indices_to_move, n * n );
            if (move)
            {
                auto algo = dynamic_cast< interactive::Algorithm< uttt::Move >* >( 
                    get_algo( uttt::game_generics, *game.current_player).get());
                if (!algo)
                    throw runtime_error( "invalid interactive uttt algorithm");
                algo->move_promise.set_value( *move );
            }
        }
        else if (ui_state == ConfigureGame)
        {
            optional< Move > move = handle_board_event( 
                rule.board.data(), game_generics.valid_moves, game.current_player->player, cell_indices_to_move, n * n );
            if (move)
            {
                if (rule.board[*move] != not_set)
                    rule.undo_move( *move, game.current_player->player );
                else 
                    rule.apply_move( *move, game.current_player->player );
                
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

namespace gui {

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
