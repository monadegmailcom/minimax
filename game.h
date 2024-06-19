#include "tree.h"

#include <iostream>
#include <chrono>

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
    MoveT get_move()
    {
        std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
        MoveT move = get_move_impl();
        std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
        duration += std::chrono::duration_cast<std::chrono::microseconds>( end - begin );
        return move;
    }
    virtual MoveT get_move_impl() = 0;

    void opponent_move( MoveT const& move )
    {
        std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
        opponent_move_impl( move );
        std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
        duration += std::chrono::duration_cast<std::chrono::microseconds>( end - begin );
    }
    virtual void opponent_move_impl( MoveT const& ) = 0;

    virtual void reset() = 0;

    std::chrono::microseconds duration {0};
};

template< typename MoveT >
struct MinimaxAlgorithm : public Algorithm< MoveT >
{
    MinimaxAlgorithm( Player player, GenericRule< MoveT >& rule,
                      std::function< double (Player) > eval,
                      std::function< bool (Minimax< MoveT > const&) > add_vertex_allowed,
                      std::function< MoveT const& (std::list< Vertex< MoveT > > const&) > choose_move,
                      bool trace )
    : player( player ), minimax( rule, eval, add_vertex_allowed ),
      choose_move( choose_move ), trace( trace )
    {}

    MoveT get_move_impl()
    {
        const double value = minimax( player );

        if (minimax.root.children.empty())
            throw std::string( "no moves");
        const MoveT move = choose_move( minimax.root.children );

        if (trace)
        {
            std::cout << "player = " << player << std::endl
                      << "recursion count = " << minimax.rec_count << std::endl
                      << "vertex count = " << minimax.vertex_count << std::endl
                      << "max depth = " << minimax.max_depth << std::endl
                      << "move = " << int( move ) << std::endl
                      << "value = " << value << std::endl << std::endl;
            tree_lens( minimax.rule, Node< MoveT >( minimax.root ), player );
            //std::ofstream gv( "tree.gv", std::ios::app );
            //PrintTree< MoveT > print_tree(
            //    gv, Node< MoveT >( minimax.root ), minimax.rule, player );
        }

        minimax.apply_move( move, player );

        if (trace)
        {
            minimax.rule.print_board( out_stream, move );
            std::cout << std::endl << std::endl;
        }

        return move;
    }

    void opponent_move_impl( MoveT const& move )
    {
        minimax.apply_move( move, Player( -player ));
    }

    void reset()
    {
        minimax.rule.reset();
        minimax.root = Vertex< MoveT >( MoveT());
    }

    const Player player;
    Minimax< MoveT > minimax;
    std::function< MoveT const& (std::list< Vertex< MoveT > > const&) > choose_move;
    const bool trace;
    OutStream out_stream {std::cout, "\e[1m", "\e[0m", "\n", " " };
};

template< typename MoveT >
struct NegamaxAlgorithm : public Algorithm< MoveT >
{
    NegamaxAlgorithm( Player player, GenericRule< MoveT >& rule, size_t depth,
        ReOrder< MoveT > reorder, std::function< double (Player) > eval, bool trace )
    : player( player ), negamax( rule ), depth( depth ), trace( trace )
    {
        negamax.reorder = reorder;
        negamax.eval = eval;
    }

    MoveT get_move_impl()
    {
        const double value = negamax( depth, player );

        assert (!negamax.moves.empty());
        MoveT const& move = negamax.moves.front();
        negamax.rule.apply_move( move, player );

        if (trace)
        {
            std::cout << "player = " << player << std::endl
                      << "recursions = " << negamax.count - acc_nodes << std::endl
                      << "value = " << value << std::endl << std::endl;
            negamax.rule.print_board( out_stream, move );
            std::cout << std::endl << std::endl;
        }

        acc_nodes = negamax.count;

        return move;
    }

    void opponent_move_impl( MoveT const& move )
    {
        negamax.rule.apply_move( move, Player( -player ));
    }

    void reset()
    {
        negamax.rule.reset();
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
Player game( GenericRule< MoveT >& rule, Algorithm< MoveT >& algo1, Algorithm< MoveT >& algo2, Player player)
{
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
    while (true)
    {
        const Player winner = rule.get_winner();
        if (winner != not_set)
            return winner;

        auto& moves = rule.generate_moves();
        if (moves.empty())
            return not_set; // draw

        MoveT const& move = algo->get_move();
        if(!any_of( moves.begin(), moves.end(), [&move](auto& m) {return m == move;} ))
        {
            std::cout << "error: " << int( move ) << " is not a valid move" << std::endl;
            break;
        }

        snd_algo->opponent_move( move );

        rule.apply_move( move, player );

        player = Player( -player );
        std::swap( algo, snd_algo );
    }

    return not_set;
}

template< typename MoveT >
void arena( GenericRule< MoveT >& rule, Algorithm< MoveT >& algo1,
    Algorithm< MoveT >& algo2, size_t rounds, bool alternate )
{
    size_t player1_wins = 0;
    size_t player2_wins = 0;
    size_t draws = 0;
    Player player = player2;
    for (size_t idx = 0; idx != rounds; ++idx)
    {
        std::cout << '.' << std::flush;

        const Player winner = game( rule, algo1, algo2, player );
        if (winner == player1)
            ++player1_wins;
        else if (winner == player2)
            ++player2_wins;
        else
            ++draws;
        algo1.reset();
        algo2.reset();
        rule.reset();
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

std::ostream& operator<<( std::ostream&, std::pair< Statistic, size_t > const& );
