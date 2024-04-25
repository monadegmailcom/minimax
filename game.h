#include "minimax.h"

template< typename MoveT >
struct Algo
{
    size_t depth;
    ReOrder< MoveT > reorder;
    Eval< MoveT > eval;
};

template< typename MoveT >
using PrintTree = std::function< void (TreeNode< MoveT > const&) >;

template< typename MoveT >
void game( Algo< MoveT > algo1, Algo< MoveT > algo2, Player player,
           Node< MoveT >& node, PrintTree< MoveT >* print_tree = 0)
{
    Algo< MoveT >& algo = algo1;

    if (player == player2)
        std::swap( algo1, algo2 );

    while (true)
    {
        Statistic stat;
        std::unique_ptr< BuildTree< MoveT > > builder;
        if (print_tree)
            builder = std::make_unique< BuildTree< MoveT > >( player );

        std::pair< double, std::optional< size_t > > result;

        // calc next move
        result = negamax(
            node, algo.depth, player2_won, player1_won, player,
            algo.eval, algo.reorder, &stat, builder.get());

        std::cout << stat.count_ << " nodes build" << std::endl;
        if (print_tree)
        {
            (*print_tree)( *(builder->root_));

            // print candidates
            TreeNode< MoveT > const& root = *(builder->root_);
            for (auto itr = root.children_.begin();
                itr != root.children_.end(); ++itr)
            {
                TreeNode< MoveT > const& child = **itr;
                assert (child.move_);
                if (child.move_ == root.best_move_)
                    std::cout << "\e[1m";
                std::cout << *child.move_ << " (" << child.value_ << ")";
                if (child.move_ == root.best_move_)
                    std::cout << "\e[0m";
                std::cout << ", ";
            }
        }

        // apply move
        if (result.second)
            node.rule.apply_move( *result.second, player);

        std::cout << std::endl;
        node.rule.print();
        std::cout << std::endl;

        if (!result.second)
        {
            Player winner = node.rule.get_winner();
            if (winner == not_set)
                std::cout << "draw" << std::endl;
            else
                std::cout << winner << " won" << std::endl;

            break;
        }

        player = Player( -player );

        std::swap( algo1, algo2 );
    }
}
