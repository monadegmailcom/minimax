#include "meta_tic_tac_toe.h"
#include "game.h"
#include "montecarlo.h"

#include <fstream>

using namespace std;
using namespace placeholders;

void run_tic_tac_toe()
{
    vector< Player > board( tic_tac_toe::n * tic_tac_toe::n, not_set );
    tic_tac_toe::Rule rule( board.data());
    Minimax< size_t > minimax( rule, true );

    Shuffle< size_t > shuffle;
    ReOrder< size_t > reorder =
        [&shuffle](Player, vector< size_t >::iterator begin,
                   vector< size_t >::iterator end)
        { shuffle( begin, end ); };
    Eval eval = [&rule](Player) { return tic_tac_toe::simple_estimate::eval( rule ); };
    ReorderByScore< size_t > reorder_by_score( rule, eval );
    montecarlo::Eval< size_t> mc_eval(
        rule, minimax.moves, 100 );

    Algo< size_t > algo1 =
        { Statistic(), 2, bind( &ReorderByScore< size_t >::operator(),
                    &reorder_by_score, _1, _2, _3), eval };
    Algo< size_t > algo2 = { Statistic(), 2, reorder, eval };

    Algo< size_t > algo3 = { Statistic(), 2,
        bind( &ReorderByScore< size_t >::operator(),
              &reorder_by_score, _1, _2, _3),
        bind( &montecarlo::Eval< size_t >::operator(),
            &mc_eval, _1 ) };

    Algo< size_t > user = {
        Statistic(), 0, [&rule]( Player, vector< size_t >::iterator begin,
                    vector< size_t >::iterator end )
           { return tic_tac_toe::user_input( rule, begin, end ); },
        [](Player) { return 0.0; }};

//    game( minimax, algo1, algo2, player1, true );

    string file_name = "tree.gv";

    ofstream file( file_name );
    if (!file)
        cerr << "opening file '" << file_name << "' failed" << endl;
    game( minimax, user, algo3, player1, true, &file );
    const size_t rounds = 2;
    //arena( minimax, algo1, algo2, player2, rounds, true);
}

void run_meta_tic_tac_toe()
{
    meta_tic_tac_toe::Rule rule;
    Minimax< size_t > minimax( rule, false );

    Shuffle< size_t > shuffle;
    ReOrder< size_t > reorder =
        [&shuffle](Player, vector< size_t >::iterator begin,
                    vector< size_t >::iterator end)
        { shuffle( begin, end ); };
    montecarlo::Eval< size_t> mc_eval(
        rule, minimax.moves, 100 );

    Eval eval1 = [&rule](Player)
        { return meta_tic_tac_toe::simple_estimate::eval( rule, 9.0 ); };
    ReorderByScore< size_t > reorder_by_score1( rule, eval1 );
    ReOrder< size_t > reorder1 =
        bind( &ReorderByScore< size_t >::operator(), &reorder_by_score1, _1, _2, _3 );

    Eval eval2 = [&rule](Player)
        { return meta_tic_tac_toe::simple_estimate::eval( rule, 9.0 ); };
    ReorderByScore< size_t > reorder_by_score2( rule, eval2 );
    ReOrder< size_t > reorder2 =
        bind( &ReorderByScore< size_t >::operator(), &reorder_by_score2, _1, _2, _3 );

    Algo< size_t > algo1 = { Statistic(), 2, reorder1, eval1 };
    Algo< size_t > algo2 = { Statistic(), 5, reorder2, eval2 };
    Algo< size_t > algo3 = { Statistic(), 1, reorder1,
        bind( &montecarlo::Eval< size_t >::operator(), &mc_eval, _1 ) };

    Algo< size_t > user = {
        Statistic(), 0, [&rule]( Player, vector< size_t >::iterator begin,
                    vector< size_t >::iterator end )
           { return meta_tic_tac_toe::user_input( rule, begin, end ); },
        [](Player) { return 0.0; }};

    string file_name = "tree.gv";

    ofstream file( file_name );
    if (!file)
        cerr << "opening file '" << file_name << "' failed" << endl;

    //game( minimax, algo2, algo1, player1, true, &file );
    OutStream out_stream {cout, "\e[1m", "\e[0m", "\n", " " };
    tic_tac_toe::Rule( rule.meta_board ).print_board( out_stream, optional< size_t >());
    const size_t rounds = 100;
    arena( minimax, algo1, algo3, player2, rounds, true);
    std::cout
         << std::endl
         << "rounds: " << rounds << std::endl
         << "player1:\n" << std::make_pair( algo1.stat, rounds ) << std::endl
         << "player2:\n" << std::make_pair( algo3.stat, rounds ) << std::endl
         << "avg node count (for both) = " << double( minimax.count ) / rounds << std::endl;
}

int main()
{
    //run_tic_tac_toe();
    run_meta_tic_tac_toe();

    return 0;
}
