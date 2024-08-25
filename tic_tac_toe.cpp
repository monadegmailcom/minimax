#include "tic_tac_toe.h"

#include <string>
#include <cassert>

using namespace std;

namespace tic_tac_toe {

std::vector< Move > Rule::moves;

Rule::Rule( Player* board ) : board( board ) {}

GenericRule< Move >* Rule::clone() const
{
    return new Rule( *this );
}

void Rule::copy_from( GenericRule< Move > const& generic_rule )
{
    Rule const* rule = dynamic_cast< Rule const* >( &generic_rule );
    if (rule)
        board = rule->board;
}

void Rule::print_move( ostream& stream, Move const& move ) const
{
    stream << size_t( move );
}

void Rule::print_board( OutStream& out_stream, optional< Move > const& last_move ) const
{
    for (size_t i = 0; i != n; ++i)
    {
        for (size_t j = 0; j != n; ++j)
        {
            const size_t idx = i * n + j;
            if (last_move && *last_move == idx)
                out_stream.stream << out_stream.emph_start;
            out_stream.stream << board[idx];
            if (j != n - 1)
                out_stream.stream << out_stream.space;
            if (last_move && *last_move == idx)
                out_stream.stream << out_stream.emph_end;
        }
        out_stream.stream << out_stream.linebreak;
    }
}

inline Player winner( size_t begin, size_t offset, Player* board )
{
    const Player player = board[begin];
    if (player == not_set)
        return not_set;
    for (size_t k = 1; k != n; ++k)
    {
        begin += offset;
        if (player != board[begin])
            return not_set;
    }
    return player;
}

Player Rule::get_winner() const
{
    Player player = not_set;
    // rows
    for (size_t k = 0; k != n; ++k)
    {
        player = winner (n * k, 1, board);
        if (player != not_set)
            return player;
    }
    // cols
    for (size_t k = 0; k != n; ++k)
    {
        player = winner (k, n, board);
        if (player != not_set)
            return player;
    }
    // diag1
    player = winner (0, n + 1, board);
    if (player != not_set)
        return player;
    // diag2
    player = winner (n - 1, n - 1, board);
    if (player != not_set)
        return player;

    return not_set;
}

vector< Move >& Rule::generate_moves()
{
    moves.clear();
    for (size_t idx = 0; idx != n * n; ++idx)
        if (board[idx] == not_set)
            moves.push_back( idx );
    return moves;
}

void Rule::apply_move( Move const& move, Player player)
{
    board[move] = player;
}

void Rule::undo_move( Move const& move, Player)
{
    board[move] = not_set;
}

namespace trivial_estimate {
    double eval( Rule const& rule )
    {
        Player winner = rule.get_winner();
        if (winner == player1)
            return player1_won;
        else if (winner == player2)
            return player2_won;
        else
            return 0.0;
    }
} // namespace trivial_estimate {

namespace simple_estimate {
    double row_player_score( Rule const& rule, const size_t i )
    {
        u_int32_t count1 = 0;
        u_int32_t count2 = 0;

        for (size_t j = 0; j != n; ++j)
        {
            Player player = rule.board[i * n + j];
            if (player == player1)
                ++count1;
            else if (player == player2)
                ++count2;
        }
        if (count1 != 0 && count2 == 0)
            return double( count1 );
        else if (count2 != 0 && count1 == 0)
            return -double( count2 );
        else
            return 0.0;
    }

    double col_player_score( Rule const& rule, const size_t j )
    {
        u_int32_t count1 = 0;
        u_int32_t count2 = 0;

        for (size_t i = 0; i != n; ++i)
        {
            Player player = rule.board[i * n + j];
            if (player == player1)
                ++count1;
            else if (player == player2)
                ++count2;
        }
        if (count1 != 0 && count2 == 0)
            return double( count1 );
        else if (count2 != 0 && count1 == 0)
            return -double( count2 );
        else
            return 0.0;
    }

    double diag1_player_score( Rule const& rule )
    {
        u_int32_t count1 = 0;
        u_int32_t count2 = 0;

        for (size_t k = 0; k != n; ++k)
        {
            Player player = rule.board[k * n + k];
            if (player == player1)
                ++count1;
            else if (player == player2)
                ++count2;
        }
        if (count1 != 0 && count2 == 0)
            return double( count1 );
        else if (count2 != 0 && count1 == 0)
            return -double( count2 );
        else
            return 0.0;
    }

    double diag2_player_score( Rule const& rule )
    {
        u_int32_t count1 = 0;
        u_int32_t count2 = 0;

        for (size_t k = 0; k != n; ++k)
        {
            Player player = rule.board[k * n + n - 1 - k];
            if (player == player1)
                ++count1;
            else if (player == player2)
                ++count2;
        }
        if (count1 != 0 && count2 == 0)
            return double( count1 );
        else if (count2 != 0 && count1 == 0)
            return -double( count2 );
        else
            return 0.0;
    }

    double eval( Rule const& rule )
    {
        double value = 0.0;
        for (size_t k = 0; k != n; ++k)
            value += row_player_score( rule, k ) + col_player_score( rule, k);

        value += diag1_player_score( rule ) + diag2_player_score( rule );

        return value;
    }

} // namespace simple_estimate {

GenericRule< Move >* DeepRule::clone() const
{
    return new Rule( *this );
}

void DeepRule::copy_from( GenericRule< Move > const& generic_rule )
{
    DeepRule const* rule = dynamic_cast< DeepRule const* >( &generic_rule );
    if (rule)
        mem = rule->mem;
}

} // namespace tic_tac_toe {
