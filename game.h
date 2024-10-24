#include "tree.h"

#include <iostream>
#include <chrono>
#include <memory>
#include <future>

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
    : player( player ), initial_rule( initial_rule.clone()) {}

    std::chrono::microseconds duration { 0 };
    const Player player;
    std::unique_ptr< GenericRule< MoveT > > initial_rule;
    std::future< MoveT > move_future;

    std::optional< MoveT > get_move()
    {
        if (!move_future.valid())
            // after the assignment the future is valid
            move_future = get_future();

        // we are polling for the player to make a move
        if (move_future.wait_for( std::chrono::milliseconds( 0 )) == std::future_status::ready)
            // this sets the future to invalid state
            return move_future.get();
        else            
            return {};
    }

    void stop() 
    { 
        stop_impl(); 
        // wait for the future to finish
        if (move_future.valid())
            move_future.wait();
    }

    virtual ~Algorithm() {}
    virtual void opponent_move( MoveT const& ) = 0;
    virtual void reset() = 0;
    virtual void stop_impl() = 0;
    virtual std::future< MoveT > get_future() = 0;
};

namespace interactive {

template< typename MoveT >
struct Algorithm : public ::Algorithm< MoveT >
{
    Algorithm( GenericRule< MoveT > const& initial_rule, Player player ) 
    : ::Algorithm< MoveT >( player, initial_rule ) {}

    void opponent_move( MoveT const& move ) 
    {}

    void reset() 
    {}

    std::future< MoveT > get_future()
    {
        move_promise = std::promise< MoveT >();
        return move_promise.get_future();
    }

    void stop_impl() 
    {
        // set some move to set future status to ready, the move will not be used
        if (this->move_future.valid())
            move_promise.set_value( MoveT());
    }

    std::promise< MoveT > move_promise;
};

} // namespace interactive {

namespace montecarlo {

template< typename MoveT >
struct ChooseMove
{
    virtual Node< MoveT > const& operator()( NodeList< MoveT > const& nodes ) const = 0;
    virtual ~ChooseMove() {}
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
    Algorithm( GenericRule< MoveT > const& initial_rule, Player player, ChooseMove< MoveT >* choose_move, 
               size_t simulations, double exploration, bool trace ) :
        ::Algorithm< MoveT >( player, initial_rule ), choose_move( choose_move ), simulations( simulations ),
        mcts( initial_rule, exploration ), trace( trace ) 
        {}

    std::future< MoveT > get_future()
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
        return std::async( 
            [this]() 
            { 
                mcts( simulations, this->player );
                const MoveT move = (*choose_move)( mcts.root.children).move;
                mcts.apply_move( move, this->player );
                return move; 
            });
       
        //if (trace)
        //    montecarlo::tree::lens( *mcts.rule, mcts.root, mcts.exploration, this->player );

        // debug
        //montecarlo::MCTS< MoveT >& mcts = dynamic_cast< montecarlo::Algorithm< MoveT >& >( *this ).mcts;
        //debug_trees.emplace_back( mcts.root );

        // debug
        //debug_trees.emplace_back( mcts.root );
    }

    void opponent_move( MoveT const& move )
    {
        mcts.apply_move( move, Player( -this->player ));
    }

    void reset()
    {
        mcts.init( *this->initial_rule );
     //   debug_trees.clear();
    }

    void stop_impl() 
    {
        mcts.stop = true;
    }

    std::unique_ptr< ChooseMove< MoveT > > choose_move;
    size_t simulations;
    MCTS< MoveT > mcts;
    const bool trace;
    //OutStream out_stream {std::cout, "\e[1m", "\e[0m", "\n", " " };
    //NodeList< MoveT > debug_trees;
};

} // namespace montecarlo {

template< typename MoveT >
struct MinimaxAlgorithm : public Algorithm< MoveT >
{
    MinimaxAlgorithm( GenericRule< MoveT > const& initial_rule, Player player,
                      std::function< double (GenericRule< MoveT >&, Player) > eval,
                      Recursion< MoveT >* recursion,
                      std::function< MoveT const& (VertexList< MoveT > const&) > choose_move,
                      bool trace )
    : ::Algorithm< MoveT >( player, initial_rule ), minimax( initial_rule, eval, *recursion ),
      choose_move( choose_move ), recursion( recursion ), trace( trace )
    {}

    std::future< MoveT > get_future()
    {
        return std::async( 
            [this]() 
            { 
                this->value = minimax( this->player );

                if (minimax.root.children.empty())
                    throw std::string( "no moves");
                const MoveT move = choose_move( minimax.root.children );
                minimax.apply_move( move, this->player );

                return move;
            });

/*        minimax.debug = [this]( Minimax< MoveT >* minimax )
            {
                std::cout << "vertice count = " << minimax->vertex_count << std::endl;
                tree_lens< MoveT >( *minimax->rule,
                    Node< MoveT >( minimax->root ), player );
            };

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
        if (trace)
        {
            minimax.rule->print_board( out_stream, move );
            std::cout << std::endl << std::endl;
        }
*/
    }

    void opponent_move( MoveT const& move )
    {
        minimax.apply_move( move, Player( -this->player ));
    }

    void reset()
    {
        minimax.rule->copy_from( *this->initial_rule );
        minimax.root = Vertex< MoveT >( MoveT());
    }

    void stop_impl() 
    {
        minimax.stop = true;
    }

    Minimax< MoveT > minimax;
    std::function< MoveT const& (VertexList< MoveT > const&) > choose_move;
    std::unique_ptr< Recursion< MoveT > > recursion;
    const bool trace;
    double value = 0.0;
//    OutStream out_stream {std::cout, "\e[1m", "\e[0m", "\n", " " };
};

template< typename MoveT >
struct NegamaxAlgorithm : public Algorithm< MoveT >
{
    NegamaxAlgorithm( GenericRule< MoveT > const& initial_rule, Player player, size_t depth,
        ReOrder< MoveT > reorder, std::function< double (GenericRule< MoveT >&, Player) > eval,
        bool trace ) : 
        ::Algorithm< MoveT >( player, initial_rule ), negamax( initial_rule, eval, reorder ), depth( depth ), trace( trace ) {}

    std::future< MoveT > get_future()
    {
        return std::async( 
            [this]() 
            { 
                this->value = negamax( depth, this->player );

                if (negamax.moves.empty())
                    throw std::string( "no moves");
                MoveT const& move = negamax.moves.front();
                negamax.rule->apply_move( move, this->player );

                return move;
            });

/*
        if (trace)
        {
            std::cout << "player = " << this->player << std::endl
                      << "recursions = " << negamax.count - acc_nodes << std::endl
                      << "value = " << value << std::endl << std::endl;
            negamax.rule->print_board( out_stream, move );
            std::cout << std::endl << std::endl;
        }
*/
    }

    void opponent_move( MoveT const& move )
    {
        negamax.rule->apply_move( move, Player( -this->player ));
    }

    void reset()
    {
        negamax.rule->copy_from( *this->initial_rule );
        negamax.moves.clear();
    }

    void stop_impl() 
    { 
        negamax.stop = true; 
    }

    Negamax< MoveT > negamax;
    size_t depth;
    const bool trace;
    double value = .0;
//    OutStream out_stream {std::cout, "\e[1m", "\e[0m", "\n", " " };
};

/*
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

        debug
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

*/