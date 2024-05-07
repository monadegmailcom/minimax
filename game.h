#include "minimax.h"

#include <iostream>

template< typename MoveT >
struct Algo
{
    Statistic stat;
    size_t depth;
    ReOrder< MoveT > reorder;
    Eval< MoveT > eval;
};

template< typename MoveT >
void game( std::vector< MoveT >& moves, Algo< MoveT >& algo1, Algo< MoveT >& algo2, Player player,
           Node< MoveT >& node, bool trace = true, PrintTree< MoveT >* print_tree = 0)
{
    Algo< MoveT >* algo = &algo1;
    Algo< MoveT >* snd_algo = &algo2;

    if (player == player2)
        std::swap( algo, snd_algo );

    while (true)
    {
        std::unique_ptr< BuildTree< MoveT > > builder;
        if (print_tree)
            builder = std::make_unique< BuildTree< MoveT > >( player );

        std::pair< double, std::optional< size_t > > result;

        // calc next move
        result = negamax(
            moves, node, algo->depth, player2_won, player1_won, player,
            algo->eval, algo->reorder, algo->stat, builder.get());

        if (trace)
            std::cout << "player " << player << ", "
                        << algo->stat.nodes << " accumulated nodes build" << std::endl;

        if (!result.second)
        {
            const Player winner = node.rule.get_winner();
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
            }
            break;
        }
        else
        {
            if (trace && print_tree)
            {
                (*print_tree)( *(builder->root_));

                // print candidates
                TreeNode< MoveT > const& root = *(builder->root_);
                std::cout << "candidates (" << root.children_.size()
                        << ")" << std::endl;
                for (auto itr = root.children_.begin();
                    itr != root.children_.end(); ++itr)
                {
                    TreeNode< MoveT > const& child = **itr;
                    assert (child.move_);
                    if (child.move_ == root.best_move_)
                        std::cout << "\e[1m";
                    node.rule.print_move( *child.move_ );
                    std::cout << " (" << child.value_ << ")";
                    if (child.move_ == root.best_move_)
                        std::cout << "\e[0m";
                    std::cout << ", ";
                }
                std::cout << std::endl;
            }

            // apply move
            node.rule.apply_move( *result.second, player);

            if (trace)
            {
                node.rule.print_board();
                std::cout << std::endl;
            }
        }

        player = Player( -player );

        std::swap( algo, snd_algo );
    }
}

template< typename MoveT >
void arena( Algo< MoveT >& algo1, Algo< MoveT >& algo2, Player player,
            Node< MoveT >& node, size_t rounds, bool alternate )
{
    std::vector< MoveT > moves;
    for (; rounds; --rounds)
    {
        node.reset();
        ++algo1.stat.rounds;
        ++algo2.stat.rounds;

        game( moves, algo1, algo2, player, node, false, (PrintTree< MoveT >*) nullptr );
        if (alternate)
            player = Player( -player );
    }
}

std::ostream& operator<<( std::ostream&, Statistic const& );
std::ostream& operator<<( std::ostream&, std::pair< Statistic, Statistic > const& );
