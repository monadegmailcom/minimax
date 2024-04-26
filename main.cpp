#include "meta_tic_tac_toe.h"
#include "game.h"

#include <fstream>

using namespace std;
using namespace placeholders;

int main()
{
    // quadratic board size
    const size_t n = 3;
    using Rule = tic_tac_toe::Rule;

    Rule rule( n );
    Node< size_t > node( rule );

    meta_tic_tac_toe::Rule mrule( n, n );
    rule.board[1] = player1;
    mrule.board[7] = make_pair( rule, not_set );
    mrule.board[5] = make_pair( rule, player2 );
    mrule.print();
    return 0;

    string file_name = "tree.gv";

    ofstream file( file_name );
    if (!file)
        cerr << "opening file '" << file_name << "' failed" << endl;

    Shuffle< size_t > shuffle;
    ReOrder< size_t > reorder = bind( &Shuffle< size_t >::operator(), &shuffle, _1, _2 );
    Algo< size_t > algo1 = { 2, reorder,
        [&rule]()
        { return tic_tac_toe::simple_estimate::eval( rule ); }};
    Algo< size_t > algo2 = { 3, reorder, []() { return 0.0; } };

    Algo< size_t > user = {
        0, [&rule]( vector< size_t >::iterator begin,
                    vector< size_t >::iterator end )
           { return tic_tac_toe::user_input( rule, begin, end ); },
        []() { return 0.0; }};

    PrintTree< size_t > print_tree = [&file, &rule]( TreeNode< size_t > const& tree_node)
        { tic_tac_toe::print_tree( file, tree_node, rule ); };

//    game( user, algo2, player1, node );
    game( user, algo2, player1, node, &print_tree );

    return 0;
}
