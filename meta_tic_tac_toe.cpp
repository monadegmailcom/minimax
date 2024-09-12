#include "meta_tic_tac_toe.h"

#include <cstdlib>
#include <cassert>

using namespace std;

namespace meta_tic_tac_toe {

std::vector< Move > Rule::moves;

Rule::Rule()
    : board {not_set},
      meta_board( board.data() + n * n * item_size ),
      terminals { false }
{}

GenericRule< Move >* Rule::clone() const
{
    auto result = new Rule();
    result->copy_from( *this );
    return result;
}

void Rule::copy_from( GenericRule< Move > const& generic_rule )
{
    Rule const* rule = dynamic_cast< Rule const* >( &generic_rule );
    if (rule)
    {
        board = rule->board;
        move_stack = rule->move_stack;
        terminals = rule->terminals;
    }
}

void Rule::update( size_t idx )
{
    Player* const inner_board = board.data() + idx * item_size;
    const Player winner = tic_tac_toe::Rule( inner_board ).get_winner();
    meta_board[idx] = winner;
    bool is_terminal = true;
    if (winner == not_set)
        for (size_t idx2 = 0; idx2 != item_size; ++idx2)
            if (inner_board[idx2] == not_set)
            {
                is_terminal = false;
                break;
            }

    terminals[idx] = is_terminal;
}

void Rule::print_move( ostream& stream, Move const& move ) const
{
    const div_t p = div( move, item_size);
    stream << p.quot << "/" << p.rem;
    /*
    const div_t outer_ij = div( p.quot, n );
    const div_t inner_ij = div( p.rem, tic_tac_toe::n);
    stream
         << "(" << outer_ij.quot + 1 << "/" << outer_ij.rem + 1
         << ", " << inner_ij.quot + 1 << "/" << inner_ij.rem + 1
         << ")" << flush;
         */
}

void Rule::print_board( OutStream& out_stream, optional< Move > const& last_move ) const
{
    const size_t mid = tic_tac_toe::n / 2;
    for (size_t i = 0; i != n; ++i)
    {
        for (size_t i2 = 0; i2 != tic_tac_toe::n; ++i2)
        {
            for (size_t j = 0; j != n; ++j)
            {
                const size_t idx = i * n + j;
                if (terminals[idx] && !move_stack.empty()
                    && move_stack.back() / item_size != idx)
                {
                    for (size_t j2 = 0; j2 != tic_tac_toe::n; ++j2)
                    {
                        if (i2 == mid && j2 == mid)
                            out_stream.stream << meta_board[idx] << out_stream.space;
                        else
                            out_stream.stream << out_stream.space << out_stream.space;
                    }
                }
                else
                    for (size_t j2 = 0; j2 != tic_tac_toe::n; ++j2)
                    {
                        const Move move = idx * item_size + i2 * tic_tac_toe::n + j2;
                        const bool is_last =
                            !move_stack.empty() && move_stack.back() == move;
                        if (is_last)
                            out_stream.stream << out_stream.emph_start;
                        out_stream.stream << board[idx * item_size + i2 * tic_tac_toe::n + j2];
                        if (is_last)
                            out_stream.stream << out_stream.emph_end;
                        out_stream.stream << out_stream.space;
                    }
                if (j != n - 1)
                    out_stream.stream << out_stream.space;
            }
            out_stream.stream << out_stream.linebreak;
        }
        if (i != n - 1)
            out_stream.stream << out_stream.linebreak;
    }
}

Player Rule::get_winner() const
{
    return tic_tac_toe::Rule( meta_board ).get_winner();
}

vector< Move >& Rule::generate_moves() const
{
    moves.clear();
    // if last move is available, the inner board is fixed
    if (!move_stack.empty())
    {
        const Move last_move = move_stack.back();
        const div_t p = div( last_move, item_size);

        // if selected inner board is not won already
        if (!terminals[p.rem])
        {
            const size_t begin = p.rem * item_size;
            const size_t end = begin + item_size;
            for (Move move = begin; move != end; ++move)
                if (board[move] == not_set)
                    moves.push_back( move );
            return moves;
        }
    }

    for (size_t idx = 0; idx != n * n; ++idx)
        if (!terminals[idx])
        {
            const size_t begin = idx * item_size;
            const size_t end = begin + item_size;
            for (Move move = begin; move != end; ++move)
                if (board[move] == not_set)
                    moves.push_back( move );
        }
    return moves;
}

void Rule::apply_move( Move const& move, Player player)
{
    board[move] = player;
    update( move / item_size );
    move_stack.push_back( move );
}

void Rule::undo_move( Move const& move, Player)
{
    board[move] = not_set;
    update( move / item_size );
    if (!move_stack.empty())
        move_stack.pop_back();
}

namespace simple_estimate {
    double eval( Rule& rule, double factor )
    {
        double value = 0.0;

        for (size_t idx = 0; idx != n * n; ++idx)
        {
            Player* const inner_board = rule.board.data() + idx * item_size;
            value += tic_tac_toe::simple_estimate::eval(
                tic_tac_toe::Rule( inner_board ));
        }
        value += factor * tic_tac_toe::simple_estimate::eval(
            tic_tac_toe::Rule( rule.meta_board ));

        return value;
    }
} // namespace simple_estimate {

} // namespace meta_tic_tac_toe {
