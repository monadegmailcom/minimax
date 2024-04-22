#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <limits>
#include <algorithm>
#include <optional>
#include <random>
#include <memory>
#include <functional>
#include <cassert>

using namespace std;
using namespace placeholders;

enum Player
{
    player1 = 1,
    player2 = -1,
    not_set = 0
};

ostream& operator<<( ostream& stream, Player player )
{
    if (player == player1)
        stream << 'X';
    else if (player == player2)
        stream << 'O';
    else
        stream << '.';
    return stream;
}

struct Node
{
    vector< Player > board;
    size_t n;
};

const double player1_won = numeric_limits< double >::infinity();
const double player2_won = -numeric_limits< double >::infinity();

void print_board( Node const& node )
{
    const size_t n = node.n;
    for (size_t i = 0; i != n; ++i)
    {
        for (size_t j = 0; j != n; ++j)
        {
            Player entry = node.board[i*n + j];
            cout << entry;
        }
        cout << '\n';
    }
}

Player get_winner( Node const& node )
{
    // rows
    for (size_t i = 0; i != node.n; ++i)
    {
        const Player player = node.board[i * node.n];
        if (player == not_set)
            continue;
        size_t count = 0;
        for (size_t j = 0; j != node.n; ++j)
            if (node.board[i * node.n + j] != player)
                break;
            else
                ++count;
        if (count == node.n)
            return player;
    }
    // cols
    for (size_t j = 0; j != node.n; ++j)
    {
        const Player player = node.board[j];
        if (player == not_set)
            continue;
        size_t count = 0;
        for (size_t i = 0; i != node.n; ++i)
            if (node.board[i * node.n + j] != player)
                break;
            else
                ++count;
        if (count == node.n)
            return player;
    }
    // diag1
    Player player = node.board[0];
    if (player != not_set)
    {
        size_t count = 0;
        for (size_t k = 0; k != node.n; ++k)
            if (node.board[k * node.n + k] != player)
                break;
            else
                ++count;
        if (count == node.n)
            return player;
    }
    // diag2
    player = node.board[node.n - 1];
    if (player != not_set)
    {
        size_t count = 0;
        for (size_t k = 0; k != node.n; ++k)
            if (node.board[k * node.n + node.n - 1 - k] != player)
                break;
            else
                ++count;
        if (count == node.n)
            return player;
    }

    return not_set;
}

namespace simple_estimate {
    double row_player_score( Node const& node, const size_t i )
    {
        const size_t n = node.n;
        Player const* a = node.board.data();
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

    double col_player_score( Node const& node, const size_t j )
    {
        const size_t n = node.n;
        Player const* a = node.board.data();
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

    double diag1_player_score( Node const& node )
    {
        const size_t n = node.n;
        Player const* a = node.board.data();
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

    double diag2_player_score( Node const& node )
    {
        const size_t n = node.n;
        Player const* a = node.board.data();
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
    double eval( Node const& node )
    {
        const size_t n = node.n;
        Player const* a = node.board.data();

        double value = 0.0;
        for (size_t k = 0; k != n; ++k)
            value += row_player_score( node, k ) + col_player_score( node, k);

        value += diag1_player_score( node ) + diag2_player_score( node );

        return value;
    }

} // namespace simple_estimate {

namespace trivial_estimate {
    double eval( Node const& node )
    {
        Player winner = get_winner( node );
        if (winner == player1)
            return player1_won;
        else if (winner == player2)
            return player2_won;
        else
            return 0.0;
    }
} // namespace trivial_estimate {

typedef function< double (Node const&) > EvalT;

void generate_moves( Node const& node, vector< size_t >& moves )
{
    const size_t n = node.n;
    Player const* a = node.board.data();

    for (size_t i = 0; i != n; ++i)
        for (size_t j = 0; j != n; ++j)
        {
            const size_t idx = i * n + j;
            Player const& player = a[idx];

            if (player == not_set)
                moves.push_back( idx );
        }
}

// reorder moves to be tried
struct Shuffle
{
    Shuffle() : g_(rd_()) {}

    void operator()(
        vector< size_t >::iterator begin, vector< size_t >::iterator end )
    {
        shuffle( begin, end, g_ );
    }

    random_device rd_;
    mt19937 g_;
};

void user_input( Node& node, Player player, vector< size_t >::iterator begin, vector< size_t >::iterator end )
{
    if (begin == end)
        return;
    size_t row, col, idx;
    do
    {
        cout << player << endl;
        cout << "row (1-" << node.n << ")? ";
        cin >> row;
        cout << "col (1-" << node.n << ")? ";
        cin >> col;
        idx = (row - 1) * node.n + col - 1;
    } while (row == 0 || col == 0 || row > node.n || col > node.n || node.board[idx] != not_set);

    auto itr = find( begin, end, idx );
    assert (itr != end);
    swap( *begin, *itr );
}

typedef function< void (Node&, Player, vector< size_t >::iterator,
                        vector< size_t >::iterator) > ReorderT;


struct Statistic
{
    Statistic() : count_( 0 ) {}

    void operator()()
    {
        ++count_;
    }

    size_t count_;
};

template< typename BuildTreeT, typename StatisticT >
pair< double, optional< size_t > > negamax(
    Node& node, size_t depth, double alpha, double beta,
    Player player, vector< size_t >& moves, EvalT eval, ReorderT& reorder,
    BuildTreeT builder, StatisticT& stat )
{
    stat();

    // new moves are appended to move container
    const size_t orig_size = moves.size();
    generate_moves( node, moves );
    const size_t new_size = moves.size();

    const Player winner = get_winner( node );
    bool is_terminal = winner != not_set
                       || new_size == orig_size;

    // apply heuristic to reorder moves
    if (!is_terminal)
        reorder( node, player, moves.begin() + orig_size, moves.begin() + new_size );

    optional< size_t > best_move;
    double value;

    if (is_terminal && winner != not_set)
        value = player * winner * player1_won;
    else if (is_terminal || !depth)
        value = player * eval( node );
    else
    {
        value = player2_won;
        for (size_t i = orig_size; i != new_size; ++i)
        {
            const size_t move = moves[i];

            // apply move
            node.board[move] = player;

            const double new_value = -negamax(
                node, depth - 1, -beta, -alpha, Player( -player ),
                moves, eval, reorder, builder( move ), stat).first;

            if (new_value > value)
            {
                value = new_value;
                best_move = move;
            }

            // undo move
            node.board[move] = not_set;

            alpha = max( alpha, value );

            if (alpha >= beta)
                break;
        }
    }

    // choose some move if no best move is set
    if (!best_move && !is_terminal)
        best_move = moves[orig_size];

    builder.update( player * value, best_move );

    // remove generated moves
    moves.resize( orig_size );

    return make_pair( value, best_move );
}

struct TreeNode
{
    TreeNode( Player player ) : player_( player ) {}
    Player player_;
    double value_;
    optional< size_t > move_;
    optional< size_t > best_move_;
    vector< shared_ptr< TreeNode > > children_;
};

struct BuildTree
{
    BuildTree( Player player )
        : root_( make_shared< TreeNode> ( player ) ) {}
    BuildTree( shared_ptr< TreeNode > tree_node )
        : root_( tree_node ) {}
    BuildTree operator()( size_t move )
    {
        shared_ptr< TreeNode > new_node( make_shared< TreeNode >(
            Player (-root_->player_ )));
        root_->children_.push_back( new_node );

        new_node->move_ = move;
        return BuildTree( new_node );
    }

    void update( double value, optional< size_t > best_move )
    {
        root_->value_ = value;
        root_->best_move_ = best_move;
    }

    void print_tree_rec(
        ostream& stream, Node& node, TreeNode const& treeNode,
        string name, bool is_best_move )
    {
        // plot vertex
        stream << name << " [label=<";
        const size_t n = node.n;
        for (size_t i = 0; i != n; ++i)
        {
            for (size_t j = 0; j != n; ++j)
            {
                const size_t idx = i * n + j;
                Player entry = node.board[idx];
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

        for (vector< shared_ptr< TreeNode > >::const_iterator
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
            node.board[move] = treeNode.player_;

            print_tree_rec(
                stream, node, **itr, new_name, is_new_best_move );

            // undo move
            node.board[move] = not_set;
        }
    }

    // does not change node
    void print( ostream& stream, Node& node  )
    {
        stream << "graph T {\n"
                << "node [fontname=\"Courier New\"]\n";
        print_tree_rec( stream, node, *root_, "v", true );
        stream << "}\n\n";
    }

    shared_ptr< TreeNode > root_;
};

struct NoBuildTree
{
    NoBuildTree operator()( size_t move ) { return NoBuildTree();}
    void update( double value, optional< size_t > best_move ) {}
};

struct Algo
{
    size_t depth;
    ReorderT reorder;
    EvalT eval;
};

void game( Algo algo1, Algo algo2, Player player, Node& node,
               vector< size_t >& moves, ostream* stream )
{
    Algo& algo = algo1;

    if (player == player2)
        swap( algo1, algo2 );

    print_board( node );

    while (true)
    {
        BuildTree build_tree( player );

        Statistic stat;
        pair< double, optional< size_t > > result;

        if (stream)
            // calc next move
            result = negamax(
                node, algo.depth, player2_won, player1_won, player, moves,
                algo.eval, algo.reorder, build_tree, stat );
        else
            result = negamax(
                node, algo.depth, player2_won, player1_won, player, moves,
                algo.eval, algo.reorder, NoBuildTree(), stat );

        if (stream)
            build_tree.print( *stream, node );

        // apply move
        if (result.second)
            node.board[*result.second] = player;

        // print candidates
        TreeNode root = *build_tree.root_;
        for (auto itr = root.children_.begin();
             itr != root.children_.end(); ++itr)
        {
            TreeNode const& child = **itr;
            assert (child.move_);
            if (child.move_ == root.best_move_)
                cout << "\e[1m";
            cout << *child.move_ << " (" << child.value_ << ")";
            if (child.move_ == root.best_move_)
                cout << "\e[0m";
            cout << ", ";
        }
        cout << endl;

        print_board( node );
        cout << endl;

        if (!result.second)
        {
            Player winner = get_winner( node );
            if (winner == not_set)
                cout << "draw" << endl;
            else
                cout << winner << " won" << endl;

            break;
        }

        player = Player( -player );

        swap( algo1, algo2 );
    }
}

int main()
{
    // quadratic board size
    size_t n = 3;
    Node node = { vector< Player >( n * n, not_set ), n };

    vector< size_t > moves;

    string file_name = "tree.gv";

    ofstream file( file_name );
    if (!file)
        cerr << "opening file '" << file_name << "' failed" << endl;

    Shuffle shuffle;
    ReorderT reorder =
        [&shuffle](Node&, Player, vector< size_t >::iterator begin,
                    vector< size_t >::iterator end) { shuffle( begin, end ); };

    Algo algo1 = { 2, reorder, &simple_estimate::eval };
    Algo algo2 = { 6, reorder, []( Node const& ) { return 0.0; } };

    Algo user = {
        0, &user_input,
        []( Node const& ) { return 0.0; },
    };

    game( user, algo2, player1, node, moves, &file );

    return 0;
}
