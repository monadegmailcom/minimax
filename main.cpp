#include "meta_tic_tac_toe.h"
#include "game.h"
#include "montecarlo.h"

#include <fstream>
#include <vector>

using namespace std;
using namespace placeholders;

void run_meta_tic_tac_toe()
{
    meta_tic_tac_toe::Rule rule1;
    meta_tic_tac_toe::Rule rule2;
    meta_tic_tac_toe::Rule game_rule;

    typedef meta_tic_tac_toe::Move Move;

    function< double (Player) > eval1 = [&rule1](Player)
        { return meta_tic_tac_toe::simple_estimate::eval( rule1, 9.0 ); };
    function< double (Player) > eval2 = [&rule2](Player)
        { return meta_tic_tac_toe::simple_estimate::eval( rule2, 9.0 ); };
    ReorderByScore< Move > reorder_by_score1( rule1, eval1 );
    Shuffle< Move > shuffle;
    ReOrder< Move > reorder1 =
        bind( &ReorderByScore< Move >::operator(), &reorder_by_score1, _1, _2, _3 );
//    ReOrder< Move > reorder1 =
//        bind( &Shuffle< Move >::operator(), &shuffle, _1, _2, _3 );

    ReorderByScore< Move > reorder_by_score2( rule2, eval2 );
    ReOrder< Move > reorder2 =
        bind( &ReorderByScore< Move >::operator(), &reorder_by_score2, _1, _2, _3 );
    function< bool (Minimax< Move > const&) > max_vertices_allowed =
        bind( &MaxVertices< Move >::operator(),
              MaxVertices< Move >( 10000 ), _1 );
    function< bool (Minimax< Move > const&) > max_depth_allowed =
        bind( &MaxDepth< Move >::operator(), MaxDepth< Move >( 7 ), _1 );
    ChooseFirst< Move > choose;
    function< Move const& (std::list< Vertex< Move > > const&) > choose_move =
        bind( &ChooseFirst< Move >::operator(), &choose, _1 );
    NegamaxAlgorithm< Move > algo1( player1, rule1, 5, reorder1, eval1, false );
    //MinimaxAlgorithm< Move > algo1( player1, rule1, eval1,
      //  max_depth_allowed, choose_move, false );
    //MinimaxAlgorithm< Move > algo2( player2, rule2, 3, reorder2, eval2, false );
    MinimaxAlgorithm< Move > algo2( player2, rule2, eval2,
        max_depth_allowed, choose_move, false );

    arena( game_rule, algo1, algo2, 100, true );
/*
    const Player winner = game( game_rule, algo1, algo2, player1 );
//    ofstream file( "tree.gv" );
//    PrintTree pt( file, algo1.minimax.vertices, rule1, player2 );

    cout << "winner = " << winner << endl
         << "player " << player1 << " nodes = " << algo1.minimax.count << endl
         << "player " << player1 << " time (sec) = "
         << algo1.duration.count() / 1000000.0 << endl
         << "player " << player2 << " time (sec) = "
         << algo2.duration.count() / 1000000.0 << endl;
    OutStream out_stream {cout, "\e[1m", "\e[0m", "\n", " " };
    cout << "resulting board" << endl;
    tic_tac_toe::Rule( game_rule.meta_board ).print_board( out_stream, optional< size_t >());

*/
/*
    MinimaxAlgorithm< Move > algo1( player1, rule1, 3, reorder1, eval1, false );
    MinimaxAlgorithm< Move > algo2( player2, rule2, 6, reorder2, eval2, false );

    arena( game_rule, algo1, algo2, player2, 10, true, true );
    cout << "player " << player1 << " time (sec) = "
         << algo1.duration.count() / 1000000.0 << endl
         << "player " << player2 << " time (sec) = "
         << algo2.duration.count() / 1000000.0 << endl;
*/
}


int main()
{
    run_meta_tic_tac_toe();

    return 0;
}
