#include "minimax.h"

using namespace std;

ostream& operator<<( ostream& stream, Statistic const& stat )
{
    stream
        << "rounds    = " << stat.rounds << endl
        << "avg nodes = " << double( stat.nodes ) / stat.rounds << endl
        << "wins      = " << stat.wins << " (" << double(stat.wins) / stat.rounds << ")" << endl
        << "losses    = " << stat.losses << " (" << double(stat.losses) / stat.rounds << ")" << endl
        << "draws     = " << stat.draws << " (" << double(stat.draws) / stat.rounds << ")" << endl;

    return stream;
}

ostream& operator<<( ostream& stream, pair< Statistic, Statistic > const& stats )
{
    const size_t sum_nodes = stats.first.nodes + stats.second.nodes;
    const size_t rounds = stats.first.rounds;
    stream
        << "stat1:" << endl << stats.first << endl
        << "stat2:" << endl << stats.second << endl
        << "avg total nodes = " << double( sum_nodes ) / rounds << endl
        << "stat1 node ratio = " << double( stats.first.nodes ) / sum_nodes << endl
        << "stat2 node ratio = " << double( stats.second.nodes ) / sum_nodes << endl;

    return stream;
}
