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
    virtual MoveT get_move() = 0;
    virtual void opponent_move( MoveT const& ) = 0;
    virtual void reset(GenericRule< MoveT >* rule) = 0;

    std::chrono::microseconds duration {0};
};

template< typename MoveT >
struct MinimaxAlgorithm : public Algorithm< MoveT >
{
    MinimaxAlgorithm( Player player,
                      std::function< double (GenericRule< MoveT >&, Player) > eval,
                      Recursion< MoveT >& recursion,
                      std::function< MoveT const& (VertexList< MoveT > const&) > choose_move,
                      bool trace )
    : player( player ), minimax( eval, recursion ),
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
        const double value = minimax( player );

        if (minimax.root.children.empty())
            throw std::string( "no moves");
        const MoveT move = choose_move( minimax.root.children );

        if (trace)
        {
            std::cout << "player = " << player << std::endl
                      << "recursion count = " << minimax.rec_count << std::endl
                      << "vertex count = " << minimax.vertex_count << std::endl
                      << "move = " << int( move ) << std::endl
                      << "value = " << value << std::endl << std::endl;
            //tree_lens( *minimax.rule, Node< MoveT >( minimax.root ), player );
            //std::ofstream gv( "tree.gv", std::ios::app );
            //PrintTree< MoveT > print_tree(
            //    gv, Node< MoveT >( minimax.root ), minimax.rule, player );
        }

        minimax.apply_move( move, player );

        if (trace)
        {
            minimax.rule->print_board( out_stream, move );
            std::cout << std::endl << std::endl;
        }

        return move;
    }

    void opponent_move( MoveT const& move )
    {
        minimax.apply_move( move, Player( -player ));
    }

    void reset( GenericRule< MoveT >* rule)
    {
        minimax.rule = rule;
        minimax.root = Vertex< MoveT >( MoveT());
    }

    const Player player;
    Minimax< MoveT > minimax;
    std::function< MoveT const& (VertexList< MoveT > const&) > choose_move;
    const bool trace;
    OutStream out_stream {std::cout, "\e[1m", "\e[0m", "\n", " " };
};

template< typename MoveT >
struct NegamaxAlgorithm : public Algorithm< MoveT >
{
    NegamaxAlgorithm( Player player, size_t depth,
        ReOrder< MoveT > reorder, std::function< double (GenericRule< MoveT >&, Player) > eval,
        bool trace )
    : player( player ), negamax( eval, reorder ), depth( depth ), trace( trace )
    {
    }

    MoveT get_move()
    {
        const double value = negamax( depth, player );

        assert (!negamax.moves.empty());
        MoveT const& move = negamax.moves.front();
        negamax.rule->apply_move( move, player );

        if (trace)
        {
            std::cout << "player = " << player << std::endl
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
        negamax.rule->apply_move( move, Player( -player ));
    }

    void reset(GenericRule< MoveT >* rule)
    {
        negamax.rule = rule;
        negamax.moves.clear();
    }

    const Player player;
    Negamax< MoveT > negamax;
    size_t depth;
    const bool trace;
    size_t acc_nodes = 0;
    OutStream out_stream {std::cout, "\e[1m", "\e[0m", "\n", " " };
};


template< typename MoveT >
std::pair< Player, std::unique_ptr< GenericRule< MoveT > > >
     game( GenericRule< MoveT >& initial_rule,
        Algorithm< MoveT >& algo1, Algorithm< MoveT >& algo2,
        Player player)
{
    std::unique_ptr< GenericRule< MoveT > > rule( initial_rule.clone());
    std::unique_ptr< GenericRule< MoveT > > rule1( initial_rule.clone());
    std::unique_ptr< GenericRule< MoveT > > rule2( initial_rule.clone());
    algo1.reset( rule1.get());
    algo2.reset( rule2.get());

    Algorithm< MoveT >* algo = nullptr;
    Algorithm< MoveT >* snd_algo = nullptr;

    assert (player != not_set);
    if (player == player1)
    {
        algo = &algo1;
        snd_algo = &algo2;
    }
    else
    {
        algo = &algo2;
        snd_algo = &algo1;
    }

    OutStream cout_stream = { std::cout, "\e[1m", "\e[0m", "\n", " " };

    std::vector< MoveT > moves;
    Player winner;
    while (true)
    {
        winner = rule->get_winner();
        if (winner != not_set)
            break;

        auto& moves = rule->generate_moves();
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

        rule->apply_move( move, player );

        player = Player( -player );
        std::swap( algo, snd_algo );
    }

    return { winner, std::move( rule ) };
}

template< typename MoveT >
void arena( GenericRule< MoveT >& initial_rule, Algorithm< MoveT >& algo1,
    Algorithm< MoveT >& algo2, size_t rounds, bool alternate )
{
    size_t player1_wins = 0;
    size_t player2_wins = 0;
    size_t draws = 0;
    Player player = player2;

    for (size_t idx = 0; idx != rounds; ++idx)
    {
        std::cout << '.' << std::flush;

        const Player winner = game( initial_rule, algo1, algo2, player ).first;
        if (winner == player1)
            ++player1_wins;
        else if (winner == player2)
            ++player2_wins;
        else
            ++draws;
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
