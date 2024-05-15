#include "meta_tic_tac_toe.h"

#include <cstdlib>

using namespace std;

namespace meta_tic_tac_toe {

Rule::Rule()
    : board {not_set},
      meta_board( board.data() + n * n * item_size ),
      board_snapshot { not_set },
      move_stack_size_snapshot( 0 ),
      terminals { false },
      terminals_snapshot { false }
{}

void Rule::reset()
{
    fill( board.begin(), board.end(), not_set );
    fill( terminals.begin(), terminals.end(), false );
    move_stack.clear();
}

void Rule::snapshot()
{
    board_snapshot = board;
    terminals_snapshot = terminals;
    move_stack_size_snapshot = move_stack.size();
}

void Rule::restore_snapshot()
{
    board = board_snapshot;
    terminals = terminals_snapshot;
    move_stack.resize( move_stack_size_snapshot );
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

void Rule::print_move( ostream& stream, size_t const& move ) const
{
    const ldiv_t p = ldiv( move, item_size);
    const ldiv_t outer_ij = ldiv( p.quot, n );
    const ldiv_t inner_ij = ldiv( p.rem, tic_tac_toe::n);
    stream
         << "(" << outer_ij.quot + 1 << "/" << outer_ij.rem + 1
         << ", " << inner_ij.quot + 1 << "/" << inner_ij.rem + 1
         << ")" << flush;
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
                        const size_t move = idx * item_size + i2 * tic_tac_toe::n + j2;
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

void Rule::generate_moves( vector< Move >& moves) const
{
    // if last move is available, the inner board is fixed
    if (!move_stack.empty())
    {
        const size_t last_move = move_stack.back();
        const ldiv_t p = ldiv( last_move, item_size);

        // if selected inner board is not won already
        if (!terminals[p.rem])
        {
            const size_t begin = p.rem * item_size;
            const size_t end = begin + item_size;
            for (size_t move = begin; move != end; ++move)
                if (board[move] == not_set)
                    moves.push_back( move );
            return;
        }
    }

    for (size_t idx = 0; idx != n * n; ++idx)
        if (!terminals[idx])
        {
            const size_t begin = idx * item_size;
            const size_t end = begin + item_size;
            for (size_t move = begin; move != end; ++move)
                if (board[move] == not_set)
                    moves.push_back( move );
        }
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
    assert (!move_stack.empty());
    move_stack.pop_back();
}

void user_input( Rule& rule,
                 vector< size_t >::iterator begin,
                 vector< size_t >::iterator end )
{
    if (begin == end)
        return;

    vector< size_t >::iterator itr;
    while (true)
    {
        size_t outer_row, outer_col, inner_row, inner_col, move;

        cout << "outer board" << endl;
        cout << "row (1-" << n << ")? ";
        cin >> outer_row;
        if (outer_row <= 0 || outer_row > n)
        {
            cout << "invalid input" << endl;
            continue;
        }
        cout << "col (1-" << n << ")? ";
        cin >> outer_col;
        if (outer_col <= 0 || outer_col > n)
        {
            cout << "invalid input" << endl;
            continue;
        }

        cout << "inner board" << endl;
        cout << "row (1-" << tic_tac_toe::n << ")? ";
        cin >> inner_row;
        if (inner_row <= 0 || inner_row > tic_tac_toe::n)
        {
            cout << "invalid input" << endl;
            continue;
        }
        cout << "col (1-" << tic_tac_toe::n << ")? ";

        cin >> inner_col;
        if (inner_col <= 0 || inner_col > tic_tac_toe::n)
        {
            cout << "invalid input" << endl;
            continue;
        }
        move = ((outer_row - 1) * n + outer_col - 1) * item_size
               + (inner_row - 1) * tic_tac_toe::n + inner_col - 1;
        itr = find( begin, end, move );
        if (itr == end)
        {
            cout << "move not possible, available moves are: ";
            for (auto itr = begin; itr != end; ++itr)
                rule.print_move( std::cout, *itr );
            cout << endl;
            continue;
        }
        break;
    }
    iter_swap( begin, itr );
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