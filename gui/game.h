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
            std::swap( current_player_algo, opponent_algo);

        if (!current_player_algo || !opponent_algo)
            throw std::runtime_error( "algo not set (process move)");

        std::optional< MoveT > move = current_player_algo->get_move();
        if (move)
        {
            if (play_mode_menu.selected == SingleStepPlayIdx)
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
            std::swap( current_player, opponent );
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
                std::swap( current_player, opponent );
                    
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
            static std::string status_text;
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
                        show_label( "tree depth", std::to_string( opponent->get_algo().get_tree_depth()).c_str());
                        if (show_button( "#120#" ))
                        {
                            if (!opponent->get_algo().get_tree_depth())
                                throw std::runtime_error( "invalid tree depth (show_side_panel)");
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
                    std::swap( current_player, opponent );

                player1->reset();
                player2->reset();

                mouse_event = MouseEvent();

                ui_state = ConfigureGame;
            }
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
