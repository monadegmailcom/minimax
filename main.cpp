#include "meta_tic_tac_toe.h"
#include "game.h"
#include "montecarlo.h"

#include <fstream>
#include <vector>

using namespace std;
using namespace placeholders;

void run_meta_tic_tac_toe()
{
    using Rule = meta_tic_tac_toe::Rule;
    typedef meta_tic_tac_toe::Move Move;

    function< double (GenericRule< Move >&, Player) > eval =
        [](GenericRule< Move >& rule, Player)
        { return meta_tic_tac_toe::simple_estimate::eval( dynamic_cast< Rule& >( rule ), 9.0 ); };
    ReorderByScore< Move > reorder_by_score( eval );
    Shuffle< Move > shuffle;
    ReOrder< Move > reorder =
        bind( &ReorderByScore< Move >::operator(), &reorder_by_score, _1, _2, _3, _4 );

    MaxVertices< Move > max_vertices( 8000 );

    MaxDepth< Move > max_depth( 7 );

    ChooseFirst< Move > choose;
    function< Move const& (std::list< Vertex< Move > > const&) > choose_move =
        bind( &ChooseFirst< Move >::operator(), &choose, _1 );

    NegamaxAlgorithm< Move > algo1( player1, 5, reorder, eval, false );
    MinimaxAlgorithm< Move > algo2( player2, eval,
        max_vertices, choose_move, false );

    Rule rule;

    arena( rule, algo1, algo2, 100, true );
    return;
    const Player winner = game( rule, algo1, algo2, player1 );
//    ofstream file( "tree.gv" );
//    PrintTree pt( file, algo1.minimax.vertices, rule1, player2 );

    cout << "winner = " << winner << endl
         << "player " << player1 << " nodes = " << algo1.negamax.count << endl
         << "player " << player1 << " time (sec) = "
         << algo1.duration.count() / 1000000.0 << endl
         << "player " << player2 << " time (sec) = "
         << algo2.duration.count() / 1000000.0 << endl;
    OutStream out_stream {cout, "\e[1m", "\e[0m", "\n", " " };
    cout << "resulting board" << endl;
    //tic_tac_toe::Rule( dynamic_cast< Rule* >( algo2.minimax.rule )->meta_board ).print_board( out_stream, optional< size_t >());

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
