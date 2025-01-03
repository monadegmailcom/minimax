#include "player.h"

namespace gui {

enum UIState { ConfigureGame, PlayGame };
enum GameIdx { TicTacToeIdx, UltimateTicTacToeIdx };

extern UIState ui_state;
extern GVC_t* gv_gvc;
extern MouseEvent mouse_event;
extern Menu game_menu;
class Game;
extern std::unique_ptr< Game > games[2];

class Game 
{
public:
    virtual ~Game();

    enum PlayModeIdx { PlayIdx, SingleStepPlayIdx, BatchPlayIdx };
    Menu play_mode_menu = Menu { "play mode", {"play", "single step play", "batch play"}, SingleStepPlayIdx };
    bool on_hold = false;

    enum ShowIdx { BoardIdx, TreeIdx };
    Menu show_menu = Menu { "show", {"board", "tree"}, TreeIdx };
    
    ::Player winner = not_set;
    Player* current_player = nullptr;
    Player* opponent = nullptr;

    void show_graph( MouseEvent& );
    void show_side_panel();
    void show_main_panel();

    virtual void process_move() = 0;
    virtual void reset() = 0;
    virtual void stop_game() = 0;
    virtual void start_game() = 0;
    virtual void show_board(UIState) = 0;
    virtual bool game_has_finished() const = 0;
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
protected:
    bool game_has_finished() const
    {
        return winner != not_set || valid_moves.empty();
    }

    virtual void reset()
    {
        last_move.reset();
        valid_moves = rule->generate_moves();
    }

    void process_move()
    {
        auto current_player_algo = player1->get_algo_generic().algorithm.get();
        auto opponent_algo = player2->get_algo_generic().algorithm.get();
        if (!current_player_algo || !opponent_algo)
            throw std::runtime_error( "algo not set (process move)");

        if (current_player == player2.get())
            std::swap( current_player_algo, opponent_algo);

        std::optional< MoveT > move = current_player_algo->get_move();
        if (move)
        {
            if (play_mode_menu.selected == SingleStepPlayIdx)
            {
                if (opponent->algo_menu.selected != Player::HumanIdx)
                    on_hold = true;

                const int selected = current_player->algo_menu.selected;
                if (selected == Player::MontecarloIdx || selected == Player::MinimaxIdx)
                    current_player->get_algo().build_tree( gv_gvc );
            }
            
            last_move = move;
            if (std::find( valid_moves.begin(), valid_moves.end(), *move) == valid_moves.end())
                throw std::runtime_error( (std::to_string( int( *move )) + " is not a valid move by " + current_player->name).c_str());
            rule->apply_move( *move, current_player->player );
            valid_moves = rule->generate_moves();

            current_player_algo->apply_move( *move );
            opponent_algo->opponent_move( *move );

            winner = rule->get_winner();
            std::swap( current_player, opponent );
        }
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
            std::optional< MoveT > move = get_move(); 
            if (move)
                process_play_move( *move );
        }
        else if (ui_state == ConfigureGame)
        {
            std::optional< MoveT > move = get_move();             
            if (move)
                process_config_move( *move );
        }
    }

    virtual void process_config_move( MoveT const& ) = 0;
    virtual void process_play_move( MoveT const& move ) = 0;
    virtual std::optional< MoveT > get_move() = 0;
protected:
    std::unique_ptr< PlayerGenerics< MoveT > > player1;
    std::unique_ptr< PlayerGenerics< MoveT > > player2;
    std::unique_ptr< GenericRule< MoveT > > rule;
    std::optional< MoveT > last_move;
    std::vector< MoveT > valid_moves;

    virtual void draw_board( float board_width) = 0;
};

class TicTacToe : public GameGenerics< tic_tac_toe::Move >
{
public:
    TicTacToe();
    void draw_board( float board_width);
    static u_int8_t cell_indices_to_move( std::pair< int, int > const& cell_indices );

    std::optional< tic_tac_toe::Move > get_move();

    void process_play_move( tic_tac_toe::Move const& move );

    void process_config_move( tic_tac_toe::Move const& move );
protected:
    void reset();
};

class MetaTicTacToe : public GameGenerics< meta_tic_tac_toe::Move >
{
public:
    MetaTicTacToe();

    void draw_board( float board_width );
    static u_int8_t cell_indices_to_move( std::pair< int, int > const& cell_indices );

    std::optional< meta_tic_tac_toe::Move > get_move();

    void process_play_move( meta_tic_tac_toe::Move const& move );
    void process_config_move( meta_tic_tac_toe::Move const& move );
    void reset();
};

} // namespace gui {
