#include "tic_tac_toe.h"

#include <iostream>

namespace meta_tic_tac_toe {

typedef u_int8_t Move;

constexpr size_t n = 3;
constexpr size_t item_size = tic_tac_toe::n * tic_tac_toe::n;
constexpr size_t board_size = (n * n + 1) * item_size;

struct Rule : public GenericRule< Move >
{
    Rule();
    void reset();
    void snapshot();
    void restore_snapshot();
    void print_move( std::ostream&, Move const& ) const;
    void print_board( OutStream&, std::optional< Move > const& last_move ) const;
    Player get_winner() const;
    std::vector< Move >& generate_moves();
    void apply_move( Move const& move, Player player);
    void undo_move( Move const& move, Player);

    void update(size_t idx);

    std::array< Player, board_size > board;
    Player* const meta_board;
    std::array< Player, board_size > board_snapshot;
    std::vector< Move > move_stack;
    size_t move_stack_size_snapshot;

    std::array< bool, item_size > terminals;
    std::array< bool, item_size > terminals_snapshot;
};

void user_input(
    Rule& rule,
    std::vector< Move >::iterator begin,
    std::vector< Move >::iterator end );

namespace simple_estimate {
double eval( Rule& rule, double factor );
} // namespace simple_estimate {

} // namespace meta_tic_tac_toe {
