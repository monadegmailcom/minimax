#pragma once

#include "negamax.h"
#include "minimax.h"
#include "montecarlo.h"

#include <iostream>
#include <chrono>
#include <memory>
#include <future>

#include <cassert>

/* struct Statistic
{
    Statistic() : wins( 0 ), draws( 0 ), losses( 0 ) {}

    size_t wins;
    size_t draws;
    size_t losses;
};

 */// TODO: do i need this?
/* class AccumulateDuration
{
public:
    AccumulateDuration( std::chrono::microseconds& duration) : 
        start( std::chrono::steady_clock::now()), duration( duration ) {}

    ~AccumulateDuration()
    { 
        duration += std::chrono::duration_cast< std::chrono::microseconds >( 
            std::chrono::steady_clock::now() - start );
    }
private:
    std::chrono::time_point< std::chrono::steady_clock > start;
    std::chrono::microseconds& duration;
};
 */

class Algorithm
{
public:
    Algorithm( Player player ) : player( player ) {}
    virtual ~Algorithm() {}
    std::chrono::microseconds get_duration() const
    {
        return duration;
    }

    std::chrono::time_point< std::chrono::steady_clock > get_start_time() const
    {
        return start;
    }

    virtual void stop() = 0;
    virtual void reset() = 0;
protected:
    const Player player;
    std::chrono::microseconds duration { 0 };
    std::chrono::time_point< std::chrono::steady_clock > start;
};

template< typename MoveT >
class AlgorithmGenerics : public Algorithm
{
public:
    AlgorithmGenerics( Player player, GenericRule< MoveT > const& initial_rule ) : 
        Algorithm( player ), initial_rule( initial_rule.clone()) {}

    virtual ~AlgorithmGenerics() {}

    std::optional< MoveT > get_move()
    {
        if (!move_future.valid())
        {
            // after the assignment the future is valid
            move_future = get_future();

            start = std::chrono::steady_clock::now(); 
        }

        // we are polling for the player to make a move
        if (move_future.wait_for( std::chrono::milliseconds( 0 )) == std::future_status::ready)
        {
            duration += std::chrono::duration_cast< std::chrono::microseconds >( 
                std::chrono::steady_clock::now() - start );

            // this sets the future to invalid state
            return move_future.get();
        }
        else            
            return {};
    }

    // we like to be able to inspect the game tree before applying the move
    void apply_move( MoveT const& move )
    {
        next_move = move;
    }

    void opponent_move( MoveT const& move ) 
    {
        opp_move = move;
    }

    void stop() 
    { 
        stop_impl(); 

        // wait for the future to finish
        if (move_future.valid())
            move_future.wait();
    }

    void reset()
    {
        reset_impl();
        duration = duration.zero();
        opp_move.reset();
    }
protected:
    std::optional< MoveT > opp_move;
    std::optional< MoveT > next_move;
    std::future< MoveT > move_future;
    std::unique_ptr< GenericRule< MoveT > > initial_rule;

    virtual void stop_impl() = 0;
    virtual void reset_impl() = 0;

    virtual std::future< MoveT > get_future() = 0;
};

namespace interactive {

template< typename MoveT >
class Algorithm : public ::AlgorithmGenerics< MoveT >
{
public:
    Algorithm( GenericRule< MoveT > const& initial_rule, Player player ) 
    : ::AlgorithmGenerics< MoveT >( player, initial_rule ) {}

    void set_move( MoveT const& move )
    {
        if (this->move_future.valid())
            this->move_promise.set_value( move );
    }
private:
    void reset_impl() {}

    std::future< MoveT > get_future()
    {
        move_promise = std::promise< MoveT >();
        return move_promise.get_future();
    }

    void stop_impl() 
    {
        // set some move to set future status to ready, the move will not be used
        set_move( MoveT());
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
class Algorithm : public ::AlgorithmGenerics< MoveT >
{
public:
    Algorithm( GenericRule< MoveT > const& initial_rule, Player player, ChooseMove< MoveT >* choose_move, 
               size_t simulations, double exploration ) :
        ::AlgorithmGenerics< MoveT >( player, initial_rule ), choose_move( choose_move ), simulations( simulations ),
        mcts( initial_rule, exploration ) 
    {}

    Node< MoveT > const& get_root()
    {
        return mcts.root;
    }

    MCTS< MoveT >& get_mcts()
    {
        return mcts;
    }
private:
    std::future< MoveT > get_future()
    {
        return std::async( 
            [this]() 
            {      
                if (this->next_move)
                    mcts.apply_move( *this->next_move, this->player );           
                if (this->opp_move)
                    mcts.apply_move( *this->opp_move, Player( -this->player ));

                mcts( simulations, this->player );
                return (*choose_move)( mcts.root.children).move;
            });
    }

    void reset_impl()
    {
        mcts.init( *this->initial_rule );
    }

    void stop_impl() 
    {
        mcts.stop = true;
    }

    std::unique_ptr< ChooseMove< MoveT > > choose_move;
    size_t simulations;
    MCTS< MoveT > mcts;
};

} // namespace montecarlo {

template< typename MoveT >
class MinimaxAlgorithm : public AlgorithmGenerics< MoveT >
{
public:
    MinimaxAlgorithm( GenericRule< MoveT > const& initial_rule, Player player,
                      std::function< double (GenericRule< MoveT >&, Player) > eval,
                      Recursion< MoveT >* recursion,
                      std::function< MoveT const& (VertexList< MoveT > const&) > choose_move ) : 
        ::AlgorithmGenerics< MoveT >( player, initial_rule ), minimax( initial_rule, eval, *recursion ),
        choose_move( choose_move ), recursion( recursion )
    {}
private:
    std::future< MoveT > get_future()
    {
        return std::async( 
            [this]() 
            { 
                if (this->next_move)
                    minimax.rule->apply_move( *this->next_move, this->player );           
                if (this->opp_move)
                    minimax.rule->apply_move( *this->opp_move, Player( -this->player ));

                this->value = minimax( this->player );

                if (minimax.root.children.empty())
                    throw std::string( "no moves");
                return choose_move( minimax.root.children );
            });
    }

    void reset_impl()
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
    double value = 0.0;
};

template< typename MoveT >
class NegamaxAlgorithm : public AlgorithmGenerics< MoveT >
{
public:
    NegamaxAlgorithm( GenericRule< MoveT > const& initial_rule, Player player, size_t depth,
        ReOrder< MoveT > reorder, std::function< double (GenericRule< MoveT >&, Player) > eval ) : 
        ::AlgorithmGenerics< MoveT >( player, initial_rule ), negamax( initial_rule, eval, reorder ), depth( depth ) {}
private:
    std::future< MoveT > get_future()
    {
        return std::async( 
            [this]() 
            { 
                if (this->next_move)
                    negamax.rule->apply_move( *this->next_move, this->player );           
                if (this->opp_move)
                    negamax.rule->apply_move( *this->opp_move, Player( -this->player ));

                this->value = negamax( depth, this->player );

                if (negamax.moves.empty())
                    throw std::string( "no moves");
                return negamax.moves.front();
            });
    }

    void reset_impl()
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
    double value = .0;
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