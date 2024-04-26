#include "meta_tic_tac_toe.h"

using namespace std;

namespace meta_tic_tac_toe {

Rule::Rule( size_t inner_n, size_t n )
    : n( n ), inner_n( inner_n ),
      board( n * n, make_pair( tic_tac_toe::Rule( inner_n ), not_set ))
{}

void Rule::print() const
{
    for (size_t i = 0; i != n; ++i)
    {
        for (size_t i2 = 0; i2 != inner_n; ++i2)
        {
            for (size_t j = 0; j != n; ++j)
            {
                auto const& item = board[i * n + j];
                auto const& inner_board = item.first.board;
                auto const& inner_result = item.second;
                const size_t mid = inner_n / 2;
                if (inner_result != not_set)
                {
                    for (size_t j2 = 0; j2 != inner_n; ++j2)
                        if (i2 == mid && j2 == mid)
                            cout << inner_result;
                        else
                            cout << ' ';
                }
                else
                    for (size_t j2 = 0; j2 != inner_n; ++j2)
                        cout << inner_board[i2 * inner_n + j2];
                cout << ' ';
            }
            cout << '\n';
        }
        cout << '\n';
    }
}

Player Rule::get_winner() const
{ return not_set; }
void Rule::generate_moves() {}
void Rule::apply_move( Move const& move, Player player) {}
void Rule::undo_move( Move const& move, Player) {}

} // namespace meta_tic_tac_toe {
