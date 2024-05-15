#include "tree.h"

#include <iostream>

struct Statistic
{
    Statistic() : wins( 0 ), draws( 0 ), losses( 0 ) {}

    size_t wins;
    size_t draws;
    size_t losses;
};

template< typename MoveT >
struct Algo
{
    Statistic stat;
    size_t depth;
    ReOrder< MoveT > reorder;
    Eval eval;
};

template< typename MoveT >
void game( Minimax< MoveT >& minimax, Algo< MoveT >& algo1, Algo< MoveT >& algo2,
           Player player, bool trace = true, std::ostream* gv = 0)
{
    Algo< MoveT >* algo = &algo1;
    Algo< MoveT >* snd_algo = &algo2;

    if (player == player2)
        std::swap( algo, snd_algo );

    OutStream cout_stream = { std::cout, "\e[1m", "\e[0m", "\n", " " };

    size_t acc_nodes = 0;
    while (true)
    {
        minimax.eval = algo->eval;
        minimax.reorder = algo->reorder;

        // calc next move
        const double value = player * minimax( algo->depth, player );

        if (gv)
            PrintTree< MoveT > print_tree( *gv, minimax.vertices, minimax.rule, player );

        if (trace)
            std::cout << minimax.count - acc_nodes << " recursions" << std::endl;

        acc_nodes = minimax.count;

        if (minimax.moves.empty())
        {
            const Player winner = minimax.rule.get_winner();
            if (winner == player1)
            {
                ++algo1.stat.wins;
                ++algo2.stat.losses;
            }
            else if (winner == player2)
            {
                ++algo2.stat.wins;
                ++algo1.stat.losses;
            }
            else
            {
                ++algo1.stat.draws;
                ++algo2.stat.draws;
            }

            if (trace)
            {
                if (winner == not_set)
                    std::cout << "draw" << std::endl;
                else
                    std::cout << winner << " won" << std::endl;

                std::cout
                    << acc_nodes << " accumulated recursions" << std::endl
                    << minimax.max_moves << " maximal move stack length" << std::endl;
            }

            break;
        }
        else
        {
            // apply move
            assert (!minimax.moves.empty());
            minimax.rule.apply_move( minimax.moves.front(), player);

            if (trace)
            {
                std::cout << "candidates (" << minimax.moves.size() << ") : ";
                for (MoveT const& move : minimax.moves)
                {
                    const bool is_best = move == minimax.moves.front();
                    if (is_best)
                        std::cout << "\e[1m";
                    minimax.rule.print_move( std::cout, move );
                    if (is_best)
                        std::cout << "\e[0m";
                    std::cout << ", ";
                }
                std::cout << "value = " << value << std::endl;
                minimax.rule.print_board( cout_stream, minimax.moves.front());
                std::cout << std::endl << std::endl;
            }

            player = Player( -player );
            std::swap( algo, snd_algo );
        }
    }
}

template< typename MoveT >
void arena( Minimax< MoveT >& minimax, Algo< MoveT >& algo1, Algo< MoveT >& algo2,
            Player player, size_t rounds, bool alternate, bool dots = true )
{
    for (size_t idx = 0; idx != rounds; ++idx)
    {
        if (dots)
            std::cout << '.' << std::flush;

        game( minimax, algo1, algo2, player, false );
        minimax.moves.clear();
        minimax.rule.reset();
        if (alternate)
            player = Player( -player );
    }
}

std::ostream& operator<<( std::ostream&, std::pair< Statistic, size_t > const& );
