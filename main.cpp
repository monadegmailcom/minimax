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
    ReOrder< size_t > reorder =
        [&shuffle](Player, vector< size_t >::iterator begin,
                   vector< size_t >::iterator end)
        { shuffle( begin, end ); };
    Eval< size_t > eval = [&rule]()
        { return tic_tac_toe::simple_estimate::eval( rule ); };
    ReorderByScore< size_t > reorder_by_score( rule, eval );

    Algo< size_t > algo1 =
        { Statistic(), 1, bind( &ReorderByScore< size_t >::operator(),
                    &reorder_by_score, _1, _2, _3), eval };
    Algo< size_t > algo2 = { Statistic(), 2, reorder, eval };

    Algo< size_t > user = {
        Statistic(), 0, [&rule]( Player, vector< size_t >::iterator begin,
                    vector< size_t >::iterator end )
           { return tic_tac_toe::user_input( rule, begin, end ); },
        []() { return 0.0; }};

    PrintTree< size_t > print_tree = [&file, &rule]( TreeNode< size_t > const& tree_node)
        { tic_tac_toe::print_tree( file, tree_node, rule ); };

    //game( moves, user, algo1, player1, node, &print_tree );
    game( moves, algo1, algo2, player1, node, true, &print_tree );
}

void run_meta_tic_tac_toe()
{
    using Rule = meta_tic_tac_toe::Rule;

    vector< size_t > moves;

    Rule rule;
    Node< size_t > node( rule );

    Shuffle< size_t > shuffle;
    ReOrder< size_t > reorder =
        [&shuffle](Player, vector< size_t >::iterator begin,
                    vector< size_t >::iterator end)
        { shuffle( begin, end ); };

    Eval< size_t > eval1 = [&rule]()
        { return meta_tic_tac_toe::simple_estimate::eval( rule, 9.0 ); };
    ReorderByScore< size_t > reorder_by_score1( rule, eval1 );
    ReOrder< size_t > reorder1 =
        bind( &ReorderByScore< size_t >::operator(), &reorder_by_score1, _1, _2, _3 );

    Eval< size_t > eval2 = [&rule]()
        { return meta_tic_tac_toe::simple_estimate::eval( rule, 9.0 ); };
    ReorderByScore< size_t > reorder_by_score2( rule, eval2 );
    ReOrder< size_t > reorder2 =
        bind( &ReorderByScore< size_t >::operator(), &reorder_by_score2, _1, _2, _3 );

    Algo< size_t > algo1 = { Statistic(), 2, reorder1, eval1 };
    Algo< size_t > algo2 = { Statistic(), 2, reorder2, eval2 };

    Algo< size_t > user = {
        Statistic(), 0, [&rule]( Player, vector< size_t >::iterator begin,
                    vector< size_t >::iterator end )
           { return meta_tic_tac_toe::user_input( rule, begin, end ); },
        []() { return 0.0; }};

    string file_name = "tree.gv";

    ofstream file( file_name );
    if (!file)
        cerr << "opening file '" << file_name << "' failed" << endl;

//    PrintTree< size_t > print_tree = [&file, &rule]( TreeNode< size_t > const& tree_node)
//        { meta_tic_tac_toe::print_tree( file, tree_node, rule ); };

    PrintTree< size_t > print_tree = []( TreeNode< size_t > const& tree_node) {};

    const size_t rounds = 2;
    //game( moves, algo1, algo2, player1, node, true, &print_tree );
    arena( algo1, algo2, player2, node, rounds, true);
}

int main()
{
    //run_tic_tac_toe();
    run_meta_tic_tac_toe();

    return 0;
}
