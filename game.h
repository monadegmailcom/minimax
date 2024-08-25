#include "tree.h"

#include <iostream>
#include <chrono>
#include <memory>
#include <cassert>

struct Statistic
{
    Statistic() : wins( 0 ), draws( 0 ), losses( 0 ) {}

    size_t wins;
    size_t draws;
    size_t losses;
};

template< typename MoveT >
struct Algorithm
{
    Algorithm( Player player, GenericRule< MoveT > const& initial_rule ) 
    : player( player ), initial_rule( initial_rule ) {}
    virtual ~Algorithm() {}
    virtual MoveT get_move() = 0;
    virtual void opponent_move( MoveT const& ) = 0;
    virtual void reset() = 0;

    std::chrono::microseconds duration { 0 };
    const Player player;
    GenericRule< MoveT > const& initial_rule;
};

namespace interactive {

template< typename MoveT >
struct Algorithm : public ::Algorithm< MoveT >
{
    Algorithm( GenericRule< MoveT > const& initial_rule, Player player ) 
    : ::Algorithm< MoveT >( player, initial_rule ), rule( initial_rule.clone()) {}

    void opponent_move( MoveT const& move ) 
    {
        rule->apply_move( move, Player( -this->player ));
        prev_move = move;
    }

    void reset() 
    { 
        rule->copy_from( this->initial_rule );
        prev_move.reset(); 
    }

    MoveT get_move()
    {
        std::cout << "current board:" << std::endl;
        rule->print_board( out_stream, prev_move );

        std::cout << "possible moves: ";
        auto moves = this->rule->generate_moves();
        for (auto move : moves)
        {
            rule->print_move( std::cout, move );
            std::cout << ", " << out_stream.space;
        }

        MoveT move;
        do
        {
            std::cout << std::endl << "your choice? ";
            move = get_user_input();
        } while (std::find( moves.begin(), moves.end(), move ) == moves.end());

        rule->apply_move( move, this->player );
        return move;
    }

    virtual MoveT get_user_input() const = 0;

    OutStream out_stream {std::cout, "\e[1m", "\e[0m", "\n", " " };
    std::optional< MoveT > prev_move;
    std::unique_ptr< GenericRule< MoveT > > rule;
};

} // namespace interactive {

namespace montecarlo {

template< typename MoveT >
struct ChooseMove
{
    virtual Node< MoveT > const& operator()( NodeList< MoveT > const& nodes ) const = 0;
};

template< typename MoveT >
struct ChooseBest : public ChooseMove< MoveT >
{
    Node< MoveT > const& operator()(NodeList< MoveT > const& nodes) const
    {
        assert (!nodes.empty());
        return *max_element( nodes.begin(), nodes.end(),
            [](auto& lhs, auto& rhs) { return lhs.denominator < rhs.denominator; });
    }
};

template< typename MoveT >
struct Algorithm : public ::Algorithm< MoveT >
{
    Algorithm( GenericRule< MoveT > const& initial_rule, Player player, ChooseMove< MoveT >& choose_move, 
               size_t simulations, double exploration, bool trace ) :
        ::Algorithm< MoveT >( player, initial_rule ), choose_move( choose_move ), simulations( simulations ),
        mcts( initial_rule, exploration ), trace( trace ) 
        {}

    MoveT get_move()
    {
//        mcts.debug = [this]( MCTS< MoveT >* mcts )
  /*      if (trace)
        {
            montecarlo::tree::lens( *mcts.rule, mcts.root, mcts.exploration, player );
        };
*/
        /* debug
        {
            montecarlo::MCTS< MoveT >& mcts = dynamic_cast< montecarlo::Algorithm< MoveT >& >( *this ).mcts;
            montecarlo::tree::PrintTree< MoveT > print( gv_before, mcts.root, mcts.exploration, *mcts.rule, player, Circular );
            montecarlo::tree::PrintTree< MoveT > print2( gv2_before, mcts.root, mcts.exploration, *mcts.rule, player, Circular, Stats );
        }*/
        mcts( simulations, this->player );
        const MoveT move = choose_move( mcts.root.children).move;

        if (trace)
        {
            std::cout << "player = " << this->player << std::endl
                      << "move   = " << int( move ) << std::endl;
            montecarlo::tree::lens( *mcts.rule, mcts.root, mcts.exploration, this->player );
        }

        // debug
        //montecarlo::MCTS< MoveT >& mcts = dynamic_cast< montecarlo::Algorithm< MoveT >& >( *this ).mcts;
        //debug_trees.emplace_back( mcts.root );

        mcts.apply_move( move, this->player );

        // debug
        //debug_trees.emplace_back( mcts.root );

        if (trace)
        {
            mcts.rule->print_board( out_stream, move );
            std::cout << std::endl << std::endl;
        }

        return move;
    }

    void opponent_move( MoveT const& move )
    {
        mcts.apply_move( move, Player( -this->player ));
    }

    void reset()
    {
        mcts.init( this->initial_rule );
     //   debug_trees.clear();
    }

    ChooseMove< MoveT >& choose_move;
    size_t simulations;
    MCTS< MoveT > mcts;
    const bool trace;
    OutStream out_stream {std::cout, "\e[1m", "\e[0m", "\n", " " };
    //NodeList< MoveT > debug_trees;
};

} // namespace montecarlo {

template< typename MoveT >
struct MinimaxAlgorithm : public Algorithm< MoveT >
{
    MinimaxAlgorithm( GenericRule< MoveT > const& initial_rule, Player player,
                      std::function< double (GenericRule< MoveT >&, Player) > eval,
                      Recursion< MoveT >& recursion,
                      std::function< MoveT const& (VertexList< MoveT > const&) > choose_move,
                      bool trace )
    : ::Algorithm< MoveT >( player, initial_rule ), minimax( initial_rule, eval, recursion ),
      choose_move( choose_move ), trace( trace )
    {}

    MoveT get_move()
    {
/*        minimax.debug = [this]( Minimax< MoveT >* minimax )
            {
                std::cout << "vertice count = " << minimax->vertex_count << std::endl;
                tree_lens< MoveT >( *minimax->rule,
                    Node< MoveT >( minimax->root ), player );
            };
*/
        const double value = minimax( this->player );

        if (minimax.root.children.empty())
            throw std::string( "no moves");
        const MoveT move = choose_move( minimax.root.children );

        if (trace)
        {
            std::cout << "player = " << this->player << std::endl
                      << "recursion count = " << minimax.rec_count << std::endl
                      << "vertex count = " << minimax.vertex_count << std::endl
                      << "move = " << int( move ) << std::endl
                      << "value = " << value << std::endl << std::endl;
            tree_lens( *minimax.rule, Node< MoveT >( minimax.root ), this->player );
            //std::ofstream gv( "tree.gv", std::ios::app );
            //PrintTree< MoveT > print_tree(
            //    gv, Node< MoveT >( minimax.root ), minimax.rule, player );
        }

        minimax.apply_move( move, this->player );

        if (trace)
        {
            minimax.rule->print_board( out_stream, move );
            std::cout << std::endl << std::endl;
        }

        return move;
    }

    void opponent_move( MoveT const& move )
    {
        minimax.apply_move( move, Player( -this->player ));
    }

    void reset()
    {
        minimax.rule->copy_from( this->initial_rule );
        minimax.root = Vertex< MoveT >( MoveT());
    }

    Minimax< MoveT > minimax;
    std::function< MoveT const& (VertexList< MoveT > const&) > choose_move;
    const bool trace;
    OutStream out_stream {std::cout, "\e[1m", "\e[0m", "\n", " " };
};

template< typename MoveT >
struct NegamaxAlgorithm : public Algorithm< MoveT >
{
    NegamaxAlgorithm( GenericRule< MoveT > const& initial_rule, Player player, size_t depth,
        ReOrder< MoveT > reorder, std::function< double (GenericRule< MoveT >&, Player) > eval,
        bool trace )
    : ::Algorithm< MoveT >( player, initial_rule ), negamax( initial_rule, eval, reorder ), depth( depth ), trace( trace ) {}

    MoveT get_move()
    {
        const double value = negamax( depth, this->player );

        assert (!negamax.moves.empty());
        MoveT const& move = negamax.moves.front();
        negamax.rule->apply_move( move, this->player );

        if (trace)
        {
            std::cout << "player = " << this->player << std::endl
                      << "recursions = " << negamax.count - acc_nodes << std::endl
                      << "value = " << value << std::endl << std::endl;
            negamax.rule->print_board( out_stream, move );
            std::cout << std::endl << std::endl;
        }

        acc_nodes = negamax.count;

        return move;
    }

    void opponent_move( MoveT const& move )
    {
        negamax.rule->apply_move( move, Player( -this->player ));
    }

    void reset()
    {
        negamax.rule->copy_from( this->initial_rule );
        negamax.moves.clear();
    }

    Negamax< MoveT > negamax;
    size_t depth;
    const bool trace;
    size_t acc_nodes = 0;
    OutStream out_stream {std::cout, "\e[1m", "\e[0m", "\n", " " };
};

template< typename MoveT >

Player game( GenericRule< MoveT >& rule,
             Algorithm< MoveT >& algo1, Algorithm< MoveT >& algo2, Player player )
{
    Algorithm< MoveT >* algo = nullptr;
    Algorithm< MoveT >* snd_algo = nullptr;

    if (algo1.player == algo2.player || player == not_set)
        throw std::runtime_error( "invalid player setting");
    else if (algo1.player == player)
    {
        algo = &algo1;
        snd_algo = &algo2;
    }
    else if (algo2.player == player)
    {
        algo = &algo2;
        snd_algo = &algo1;
    }

    algo1.reset();
    algo2.reset();

    OutStream cout_stream = { std::cout, "\e[1m", "\e[0m", "\n", " " };

    std::vector< MoveT > moves;
    Player winner;

    while (true)
    {
        winner = rule.get_winner();
        if (winner != not_set)
            break;

        // make a copy!
        moves = rule.generate_moves();
        if (moves.empty())
            break; // draw

        auto begin = std::chrono::steady_clock::now();
        MoveT move = algo->get_move();
        auto end = std::chrono::steady_clock::now();

        algo->duration += std::chrono::duration_cast<std::chrono::microseconds>( end - begin );

        if(!any_of( moves.begin(), moves.end(), [&move](auto& m) {return m == move;} ))
        {
            std::cout << "error: " << int( move ) << " is not a valid move by " << player
                      << std::endl;
            winner = not_set;
            break;
        }

        begin = std::chrono::steady_clock::now();
        snd_algo->opponent_move( move );
        end = std::chrono::steady_clock::now();
        snd_algo->duration += std::chrono::duration_cast<std::chrono::microseconds>( end - begin );

        rule.apply_move( move, player );

        player = Player( -player );
        std::swap( algo, snd_algo );
    }

    return winner;
}

template< typename MoveT >
void arena( GenericRule< MoveT > const& initial_rule, GenericRule< MoveT >& rule, Algorithm< MoveT >& algo1,
            Algorithm< MoveT >& algo2, size_t rounds, bool alternate )
{
    size_t player1_wins = 0;
    size_t player2_wins = 0;
    size_t draws = 0;
    Player player = player1;

    for (size_t idx = 0; idx != rounds; ++idx)
    {
        std::cout << '.' << std::flush;
        rule.copy_from( initial_rule );
        const Player winner = game( rule, algo1, algo2, player );
        if (winner == player1)
            ++player1_wins;
        else if (winner == player2)
            ++player2_wins;
        else
            ++draws;

        /* debug
        //if (winner == player1)
        {
            std::cout << "player " << winner << " won" << std::endl;
            player = player1;

            auto& algo = dynamic_cast< montecarlo::Algorithm< MoveT >& >( algo2 );

            std::unique_ptr< GenericRule< MoveT > > rule( initial_rule.clone());
            for (montecarlo::Node< MoveT > const& node : algo.debug_trees)
            {
                rule->apply_move( node.move, player );
                player = Player( -player );
                montecarlo::tree::lens( *rule, node, algo.mcts.exploration, player );
            }

            break;
        }
        */
        if (alternate)
            player = Player( -player );
    }

    std::cout << std::endl
              << "player " << player1 << " wins = " << player1_wins << std::endl
              << "player " << player1 << " duration = "
              << algo1.duration.count() / 1000000.0 << std::endl
              << "player " << player2 << " wins = " << player2_wins << std::endl
              << "player " << player2 << " duration = "
              << algo2.duration.count() / 1000000.0 << std::endl
              << "draws = " << draws << std::endl;
}
