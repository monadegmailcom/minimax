#include "player.h"

#include <limits>

using namespace std;

const double player1_won = numeric_limits< double >::infinity();
const double player2_won = -numeric_limits< double >::infinity();

ostream& operator<<( ostream& stream, Player player )
{
    if (player == player1)
        stream << 'X';
    else if (player == player2)
        stream << 'O';
    else
        stream << '.';
    return stream;
}
