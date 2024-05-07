#include "meta_tic_tac_toe.h"

#include <cstdlib>

using namespace std;

namespace meta_tic_tac_toe {

Rule::Rule()
    : board {not_set},
      meta_board( board.data() + n * n * item_size ),
      terminals { false }
{}

void Rule::reset()
{
    fill( board.begin(), board.end(), not_set );
    fill( terminals.begin(), terminals.end(), false );
    move_stack.clear();
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

void Rule::print_move( size_t const& move ) const
{
    const ldiv_t p = ldiv( move, item_size);
    const ldiv_t outer_ij = ldiv( p.quot, n );
    const ldiv_t inner_ij = ldiv( p.rem, tic_tac_toe::n);
    cout << "(" << outer_ij.quot + 1 << "/" << outer_ij.rem + 1
           << ", " << inner_ij.quot + 1 << "/" << inner_ij.rem + 1
           << ")" << flush;
}

void print_board_impl(
    Rule const& rule, ostream& stream, string const& linebreak,
    string const& emph_start, string const& emph_end )
{
    const size_t mid = tic_tac_toe::n / 2;
    for (size_t i = 0; i != n; ++i)
    {
        for (size_t i2 = 0; i2 != tic_tac_toe::n; ++i2)
        {
            for (size_t j = 0; j != n; ++j)
            {
                const size_t idx = i * n + j;
                if (rule.terminals[idx] && !rule.move_stack.empty()
                    && rule.move_stack.back() / item_size != idx)
                {
                    for (size_t j2 = 0; j2 != tic_tac_toe::n; ++j2)
                        if (i2 == mid && j2 == mid)
                            stream << rule.meta_board[idx] << ' ';
                        else
                            stream << "  ";
                }
                else
                    for (size_t j2 = 0; j2 != tic_tac_toe::n; ++j2)
                    {
                        const size_t move = idx * item_size + i2 * tic_tac_toe::n + j2;
                        const bool is_last =
                            !rule.move_stack.empty() && rule.move_stack.back() == move;
                        if (is_last)
                            stream << emph_start;
                        stream << rule.board[idx * item_size + i2 * tic_tac_toe::n + j2] << ' ';
                        if (is_last)
                            stream << emph_end;
                    }
                stream << ' ';
            }
            stream << linebreak;
        }
        stream << linebreak;
    }
}

void Rule::print_board() const
{
    print_board_impl( *this, cout, "\n", "\e[1m", "\e[0m" );

    bool is_terminal = true;
    for (bool terminal : terminals)
        if (!terminal)
        {
            is_terminal = false;
            break;
        }
    if (is_terminal || get_winner() != not_set)
        tic_tac_toe::Rule( meta_board ).print_board();
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
                rule.print_move( *itr );
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

void print_tree_rec(
    ostream& stream, TreeNode< size_t > const& treeNode,
    Rule& rule, string name, bool is_best_move )
{
    Player* board = rule.board.data();

    // plot vertex
    stream << name << " [label=<";
    print_board_impl( rule, stream, "<BR/>", "<B>", "</B>");
    stream << ">";
    if (is_best_move)
        stream << " color=\"red\"";

    if (treeNode.player_ == player1)
        stream << " shape=box";
    stream << "]\n";

    for (vector< shared_ptr< TreeNode< size_t > > >::const_iterator
            itr = treeNode.children_.begin();
            itr != treeNode.children_.end(); ++itr)
    {
        assert ((**itr).move_);
        const size_t move = *(**itr).move_;

        const string new_name = name + "_" + to_string( move );
        bool is_new_best_move =
            is_best_move && move == treeNode.best_move_;

        // plot edge to child
        stream << name << " -- " << new_name
            << " [label=\"" << (**itr).value_ << "\"";
        if (is_new_best_move)
            stream << " color=\"red\"";
        stream << "]\n";

        // apply move
        rule.apply_move( move, treeNode.player_);

        print_tree_rec(
            stream, **itr, rule, new_name, is_new_best_move );

        // undo move
        rule.undo_move( move, treeNode.player_ );
    }
}

void print_tree( ostream& stream, TreeNode< size_t > const& root,
                 Rule& rule )
{
    stream << "graph T {\n"
            << "node [fontname=\"Courier New\"]\n";
    print_tree_rec( stream, root, rule, "v", true );
    stream << "}\n\n";
}

} // namespace meta_tic_tac_toe {
