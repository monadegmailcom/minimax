#include <iostream>

enum Player
{
    player1 = 1,
    player2 = -1,
    not_set = 0
};

extern const double player1_won;
extern const double player2_won;

std::ostream& operator<<( std::ostream& stream, Player player );
