#include "game.h"

using namespace std;

ostream& operator<<( ostream& stream, pair< Statistic, size_t > const& p )
{
    Statistic const& stat = p.first;
    const size_t rounds = p.second;
    assert (rounds);

    stream
        << "wins      = " << stat.wins << " (" << double(stat.wins) / rounds << ")" << endl
        << "losses    = " << stat.losses << " (" << double(stat.losses) / rounds << ")" << endl
        << "draws     = " << stat.draws << " (" << double(stat.draws) / rounds << ")" << endl;

    return stream;
}
