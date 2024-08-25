#include "meta_tic_tac_toe.h"
#include "game.h"

#include <fstream>
#include <vector>

using namespace std;
using namespace placeholders;

namespace meta_tic_tac_toe {

struct InteractiveAlgorithm : public interactive::Algorithm< Move >
{
    InteractiveAlgorithm( GenericRule< Move > const& initial_rule, Player player ) 
    : interactive::Algorithm< Move >( initial_rule, player ) {}

    Move get_user_input() const override
    {
        string input;
        cout << "meta: ";
        cin >> input;
        int move = stoi( input ) * 9;
        cout << "inner: ";
        cin >> input;
        move += stoi( input );

        return Move( move );
    }
};

} // namespace meta_tic_tac_toe {

void run_meta_tic_tac_toe()
{
    using Rule = meta_tic_tac_toe::Rule;
    typedef meta_tic_tac_toe::Move Move;

    Rule rule;
    Rule initial_rule;

    function< double (GenericRule< Move >&, Player) > eval =
        [](GenericRule< Move >& rule, Player)
        { return meta_tic_tac_toe::simple_estimate::eval( dynamic_cast< Rule& >( rule ), 9.0 ); };
    ReorderByScore< Move > reorder_by_score( eval );
    Shuffle< Move > shuffle;
    ReOrder< Move > reorder =
        bind( &ReorderByScore< Move >::operator(), &reorder_by_score, _1, _2, _3, _4 );

    MaxVertices< Move > max_vertices( 280000 );

    MaxDepth< Move > max_depth( 7 );

    ChooseFirst< Move > choose;
    function< Move const& (VertexList< Move > const&) > choose_move =
        bind( &ChooseFirst< Move >::operator(), &choose, _1 );

    NegamaxAlgorithm< Move > algo1( initial_rule, player1, 7, reorder, eval, false );
    MinimaxAlgorithm< Move > algo2( initial_rule, player2, eval, max_vertices, choose_move, false );

    meta_tic_tac_toe::InteractiveAlgorithm user( initial_rule, player2 );

    montecarlo::ChooseBest< Move > choose_best;
    montecarlo::Algorithm< Move > monte_algo( initial_rule, player1, choose_best, 8000, 0.3, false );
    montecarlo::Algorithm< Move > monte_algo2( initial_rule, player1, choose_best, 80000, 0.4, true ); // best exploration factor
    //montecarlo::Algorithm< Move > monte_algo2( player2, choose_best, 80, 0.4, false ); // best exploration factor

    //arena( initial_rule, rule, monte_algo2, algo2, 100, true );
    //return;
    
    const Player winner = game( rule, user, monte_algo2, player1 );

    cout << "winner = " << winner << endl
         << "player " << player1 << " nodes = " << algo1.negamax.count << endl
         << "player " << player1 << " time (sec) = "
         << algo1.duration.count() / 1000000.0 << endl
         << "player " << player2 << " time (sec) = "
         << algo2.duration.count() / 1000000.0 << endl;
    OutStream out_stream {cout, "\e[1m", "\e[0m", "\n", " " };
    cout << "resulting board" << endl;
    tic_tac_toe::Rule( rule.meta_board ).print_board( out_stream, optional< size_t >());
}

namespace tic_tac_toe {

struct InteractiveAlgorithm : public interactive::Algorithm< Move >
{
    InteractiveAlgorithm( GenericRule< Move > const& initial_rule, Player player ) 
    : interactive::Algorithm< Move >( initial_rule, player ) {}

    tic_tac_toe::Move get_user_input() const override
    {
        string input;
        cin >> input;
        return Move( stoi( input ));
    }
};

} // namespace tic_tac_toe {

void monte()
{
    using Rule = tic_tac_toe::DeepRule;
    typedef tic_tac_toe::Move Move;

    Rule rule;
    Rule initial_rule;

    function< double (GenericRule< Move >&, Player) > eval =
        [](GenericRule< Move >& rule, Player)
        { return tic_tac_toe::simple_estimate::eval( dynamic_cast< Rule& >( rule )); };
    ReorderByScore< Move > reorder_by_score( eval );
    ReOrder< Move > reorder =
        bind( &ReorderByScore< Move >::operator(), &reorder_by_score, _1, _2, _3, _4 );

    NegamaxAlgorithm< Move > algo1( initial_rule, player1, 4, reorder, eval, false );
//    NegamaxAlgorithm< Move > algo1( player1, 5, reorder, eval, false );
    NegamaxAlgorithm< Move > algo3( initial_rule, player1, 2, reorder, eval, false );

    montecarlo::ChooseBest< Move > choose;
    montecarlo::Algorithm< Move > algo2( initial_rule, player2, choose, 2000, 1.41, true );

    //arena( rule, algo1, algo2, 1000, false );
    //return;

    tic_tac_toe::InteractiveAlgorithm user( initial_rule, player1 );

    const Player winner = game( rule, user, algo2, player2 );

    cout << "winner = " << winner << endl
         << "player " << player1 << " nodes = " << algo1.negamax.count << endl
         << "player " << player1 << " time (sec) = "
         << algo1.duration.count() / 1000000.0 << endl
         << "player " << player2 << " time (sec) = "
         << algo2.duration.count() / 1000000.0 << endl;
    OutStream out_stream {cout, "\e[1m", "\e[0m", "\n", " " };
    cout << "resulting board" << endl;
    rule.print_board( out_stream, optional< size_t >());
}

int main()
{
    run_meta_tic_tac_toe();
    //monte();

    return 0;
}
