#include "rule.h"

namespace montecarlo {
template< typename MoveT >
struct Eval
{
    Eval( GenericRule< MoveT >& rule, std::vector< MoveT >& moves,
          size_t simulations )
        : rule( rule ), moves( moves ), simulations( simulations ), gen(rd())
    {}

    size_t random( size_t min, size_t max )
    {
        return std::uniform_int_distribution<std::mt19937::result_type>(
            min, max)( rd );
    }

    // value between -1.0 and 1.0
    double operator()(Player player)
    {
        rule.snapshot();
        size_t player1_wins = 0;
        size_t player1_losses = 0;
        for (size_t idx = simulations; idx; --idx)
        {
            Player winner;
            Player pl = player;

            while (true)
            {
                winner = rule.get_winner();
                if (winner != not_set)
                    break;

                const size_t prev_size = moves.size();
                rule.generate_moves( moves );
                const size_t new_size = moves.size();
                if (new_size == prev_size)
                    break;

                rule.apply_move( moves[random(prev_size, new_size - 1)], pl );
                moves.resize( prev_size );

                pl = Player( -pl );
            }

            if (winner == player1)
                ++player1_wins;
            else if (winner == player2)
                ++player1_losses;

            rule.restore_snapshot();
        }

        return (double(player1_wins) - player1_losses) / simulations;
    }

    GenericRule< MoveT >& rule;
    std::vector< MoveT >& moves;
    const size_t simulations;
    std::random_device rd;
    std::mt19937 gen;
};

} // namespace montecarlo {
