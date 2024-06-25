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
    function< Move const& (VertexList< Move > const&) > choose_move =
        bind( &ChooseFirst< Move >::operator(), &choose, _1 );

    NegamaxAlgorithm< Move > algo1( player1, 5, reorder, eval, false );
    MinimaxAlgorithm< Move > algo2( player2, eval,
        max_depth, choose_move, false );

    Rule rule;

    arena( rule, algo1, algo2, 100, true );
    return;
    const pair< Player, unique_ptr< GenericRule< Move > > > p = game( rule, algo1, algo2, player1 );
    const Player winner = p.first;
    Rule* res_rule = dynamic_cast< Rule* >( p.second.get());
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
    tic_tac_toe::Rule( res_rule->meta_board ).print_board( out_stream, optional< size_t >());
}

int main()
{
    run_meta_tic_tac_toe();

    return 0;
}
