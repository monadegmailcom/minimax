#include "minimax.h"

#include <array>

namespace tic_tac_toe {

typedef size_t Move;

constexpr size_t n = 3;

struct Rule : public GenericRule< Move >
{
    Rule(Player*);
    void reset();
    void snapshot();
    void print_move( std::ostream&, Move const& ) const;
    void print_board( OutStream&, std::optional< Move > const& last_move ) const;
    Player get_winner() const;
    void generate_moves( std::vector< Move >& ) const;
    void apply_move( Move const& move, Player player);
    void undo_move( Move const& move, Player);

    Player* const board;
    std::array< Player, n * n > board_snapshot;
};

namespace trivial_estimate {
double eval( Rule const& rule );
} // namespace trivial_estimate {

namespace simple_estimate {
double eval( Rule const& rule );
} // namespace simple_estimate {

void user_input( Rule& rule,
    std::vector< size_t >::iterator begin,
    std::vector< size_t >::iterator end );

} // namespace tic_tac_toe {
