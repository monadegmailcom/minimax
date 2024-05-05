#include "meta_tic_tac_toe.h"
#include "game.h"

#include <fstream>

using namespace std;
using namespace placeholders;

void run_tic_tac_toe()
{
    vector< size_t > moves;

    using Rule = tic_tac_toe::Rule;

    vector< Player > board( tic_tac_toe::n * tic_tac_toe::n, not_set );
    Rule rule( board.data());
    Node< size_t > node( rule );

    string file_name = "tree.gv";

    ofstream file( file_name );
    if (!file)
        cerr << "opening file '" << file_name << "' failed" << endl;

    Shuffle< size_t > shuffle;
    ReOrder< size_t > reorder = bind( &Shuffle< size_t >::operator(), &shuffle, _1, _2 );
    Algo< size_t > algo1 = { 1, reorder,
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

    //game( moves, user, algo1, player1, node, &print_tree );
    game( moves, algo1, algo2, player1, node, &print_tree );
}

void run_meta_tic_tac_toe()
{
    using Rule = meta_tic_tac_toe::Rule;

    vector< size_t > moves;
//    ofstream pipe( "pipe" );
//    if (!pipe)
//        cerr << "cannot open pipe" << endl;

    Rule rule;
    Node< size_t > node( rule );

    Shuffle< size_t > shuffle;
    ReOrder< size_t > reorder = bind( &Shuffle< size_t >::operator(), &shuffle, _1, _2 );
    Algo< size_t > algo1 = { 2, reorder,
        [&rule]()
        { return meta_tic_tac_toe::simple_estimate::eval( rule, 3.0 ); }};
    Algo< size_t > algo2 = { 2, reorder,
        [&rule]()
        { return meta_tic_tac_toe::simple_estimate::eval( rule, 3.0 ); }};

    Algo< size_t > user = {
        0, [&rule]( vector< size_t >::iterator begin,
                    vector< size_t >::iterator end )
           { return meta_tic_tac_toe::user_input( rule, begin, end ); },
        []() { return 0.0; }};

    PrintTree< size_t > print_tree = []( TreeNode< size_t > const& tree_node) {};

    game( moves, algo1, algo2, player1, node, &print_tree );
}

int main()
{
    //run_tic_tac_toe();
    run_meta_tic_tac_toe();

    return 0;
}
