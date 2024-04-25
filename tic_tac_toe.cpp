#include "game.h"

#include <fstream>
#include <string>
#include <cassert>

using namespace std;
using namespace placeholders;

namespace tic_tac_toe {

struct Rule : public GenericRule< size_t >
{
    Rule( size_t n ) : n( n ), board( n * n, not_set ) {}

    typedef size_t Move;
    typedef vector< Player > State;

    size_t n;
    vector< Player > board;

    void print()
    {
        for (size_t i = 0; i != n; ++i)
        {
            for (size_t j = 0; j != n; ++j)
            {
                Player entry = board[i * n + j];
                cout << entry;
            }
            cout << '\n';
        }
    }

    Player get_winner() const
    {
        // rows
        for (size_t i = 0; i != n; ++i)
        {
            const Player player = board[i * n];
            if (player == not_set)
                continue;
            size_t count = 0;
            for (size_t j = 0; j != n; ++j)
                if (board[i * n + j] != player)
                    break;
                else
                    ++count;
            if (count == n)
                return player;
        }
        // cols
        for (size_t j = 0; j != n; ++j)
        {
            const Player player = board[j];
            if (player == not_set)
                continue;
            size_t count = 0;
            for (size_t i = 0; i != n; ++i)
                if (board[i * n + j] != player)
                    break;
                else
                    ++count;
            if (count == n)
                return player;
        }
        // diag1
        Player player = board[0];
        if (player != not_set)
        {
            size_t count = 0;
            for (size_t k = 0; k != n; ++k)
                if (board[k * n + k] != player)
                    break;
                else
                    ++count;
            if (count == n)
                return player;
        }
        // diag2
        player = board[n - 1];
        if (player != not_set)
        {
            size_t count = 0;
            for (size_t k = 0; k != n; ++k)
                if (board[k * n + n - 1 - k] != player)
                    break;
                else
                    ++count;
            if (count == n)
                return player;
        }

        return not_set;
    }

    void generate_moves()
    {
        for (size_t i = 0; i != n; ++i)
            for (size_t j = 0; j != n; ++j)
            {
                const size_t move = i * n + j;

                if (board[move] == not_set)
                    moves.push_back( move );
            }
    }

    void apply_move(Move const& move, Player player)
    {
        board[move] = player;
    }

    void undo_move(Move const& move, Player)
    {
        board[move] = not_set;
    }
};

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
        const size_t n = rule.n;
        Player const* a = rule.board.data();
        size_t count1 = 0;
        size_t count2 = 0;

        for (size_t j = 0; j != n; ++j)
        {
            Player player = a[i * n + j];
            if (player == player1)
                ++count1;
            else if (player == player2)
                ++count2;
        }
        if (count1 == n)
            return player1_won;
        else if (count2 == n)
            return player2_won;
        else if (count1 != 0 && count2 == 0)
            return 1.0;
        else if (count2 != 0 && count1 == 0)
            return -1.0;
        else
            return 0.0;
    }

    double col_player_score( Rule const& rule, const size_t j )
    {
        const size_t n = rule.n;
        Player const* a = rule.board.data();
        size_t count1 = 0;
        size_t count2 = 0;

        for (size_t i = 0; i != n; ++i)
        {
            Player player = a[i * n + j];
            if (player == player1)
                ++count1;
            else if (player == player2)
                ++count2;
        }
        if (count1 == n)
            return player1_won;
        else if (count2 == n)
            return player2_won;
        else if (count1 != 0 && count2 == 0)
            return 1.0;
        else if (count2 != 0 && count1 == 0)
            return -1.0;
        else
            return 0.0;
    }

    double diag1_player_score( Rule const& rule )
    {
        const size_t n = rule.n;
        Player const* a = rule.board.data();
        size_t count1 = 0;
        size_t count2 = 0;

        for (size_t k = 0; k != n; ++k)
        {
            Player player = a[k * n + k];
            if (player == player1)
                ++count1;
            else if (player == player2)
                ++count2;
        }
        if (count1 == n)
            return player1_won;
        else if (count2 == n)
            return player2_won;
        else if (count1 != 0 && count2 == 0)
            return 1.0;
        else if (count2 != 0 && count1 == 0)
            return -1.0;
        else
            return 0.0;
    }

    double diag2_player_score( Rule const& rule )
    {
        const size_t n = rule.n;
        Player const* a = rule.board.data();
        size_t count1 = 0;
        size_t count2 = 0;

        for (size_t k = 0; k != n; ++k)
        {
            Player player = a[k * n + n - 1 - k];
            if (player == player1)
                ++count1;
            else if (player == player2)
                ++count2;
        }
        if (count1 == n)
            return player1_won;
        else if (count2 == n)
            return player2_won;
        else if (count1 != 0 && count2 == 0)
            return 1.0;
        else if (count2 != 0 && count1 == 0)
            return -1.0;
        else
            return 0.0;
    }

    // undefined, if both players have wins
    double eval( Rule const& rule )
    {
        const size_t n = rule.n;
        Player const* a = rule.board.data();

        double value = 0.0;
        for (size_t k = 0; k != n; ++k)
            value += row_player_score( rule, k ) + col_player_score( rule, k);

        value += diag1_player_score( rule ) + diag2_player_score( rule );

        return value;
    }

} // namespace simple_estimate {

void user_input( Rule& rule,
                 vector< size_t >::iterator begin,
                 vector< size_t >::iterator end )
{
    if (begin == end)
        return;

    vector< Player >& board = rule.board;
    rule.print();

    size_t row, col, idx;
    do
    {
        cout << "row (1-" << rule.n << ")? ";
        cin >> row;
        cout << "col (1-" << rule.n << ")? ";
        cin >> col;
        idx = (row - 1) * rule.n + col - 1;
    } while (row == 0 || col == 0 || row > rule.n || col > rule.n || board[idx] != not_set);

    auto itr = find( begin, end, idx );
    assert (itr != end);
    iter_swap( begin, itr );
}

void print_tree_rec(
    ostream& stream, TreeNode< size_t > const& treeNode,
    Rule& rule, string name, bool is_best_move )
{
    vector< Player >& board = rule.board;
    const size_t n = rule.n;

    // plot vertex
    stream << name << " [label=<";
    for (size_t i = 0; i != n; ++i)
    {
        for (size_t j = 0; j != n; ++j)
        {
            const size_t idx = i * n + j;
            Player entry = board[idx];
            if (idx == treeNode.move_)
                stream << "<B>";
            stream << entry;
            if (idx == treeNode.move_)
                stream << "</B>";
        }
        stream << "<BR/>";
    }
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
        board[move] = treeNode.player_;

        print_tree_rec(
            stream, **itr, rule, new_name, is_new_best_move );

        // undo move
        board[move] = not_set;
    }
}

// does not change node
void print_tree( ostream& stream, TreeNode< size_t > const& root,
                 Rule& rule )
{
    stream << "graph T {\n"
            << "node [fontname=\"Courier New\"]\n";
    print_tree_rec( stream, root, rule, "v", true );
    stream << "}\n\n";
}

} // namespace tic_tac_toe {

int main()
{
    // quadratic board size
    const size_t n = 3;
    using Rule = tic_tac_toe::Rule;

    Rule rule( n );
    Node< size_t > node( rule );

    string file_name = "tree.gv";

    ofstream file( file_name );
    if (!file)
        cerr << "opening file '" << file_name << "' failed" << endl;

    Shuffle< size_t > shuffle;
    ReOrder< size_t > reorder = bind( &Shuffle< size_t >::operator(), &shuffle, _1, _2 );
    Algo< size_t > algo1 = { 2, reorder,
        [&rule]()
        { return tic_tac_toe::simple_estimate::eval( rule ); }};
    Algo< size_t > algo2 = { 6, reorder, []() { return 0.0; } };

    Algo< size_t > user = {
        0, [&rule]( vector< size_t >::iterator begin,
                    vector< size_t >::iterator end )
           { return tic_tac_toe::user_input( rule, begin, end ); },
        []() { return 0.0; }};

    PrintTree< size_t > print_tree = [&file, &rule]( TreeNode< size_t > const& tree_node)
        { tic_tac_toe::print_tree( file, tree_node, rule ); };

//    game( user, algo2, player1, node );
    game( algo1, algo2, player1, node );

    return 0;
}
