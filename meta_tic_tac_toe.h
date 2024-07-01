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
    GenericRule< Move >* clone( std::vector< unsigned char >* = nullptr ) const;
    size_t size_of() const { return sizeof( Rule ); }
    void print_move( std::ostream&, Move const& ) const;
    void print_board( OutStream&, std::optional< Move > const& last_move ) const;
    Player get_winner() const;
    std::vector< Move >& generate_moves();
    void apply_move( Move const& move, Player);
    void undo_move( Move const& move, Player);

    void update(size_t idx);

    std::array< Player, board_size > board;
    Player* meta_board;
    std::vector< Move > move_stack;
    std::array< bool, item_size > terminals;
    std::vector< Move > moves;
};

void user_input(
    Rule& rule,
    std::vector< Move >::iterator begin,
    std::vector< Move >::iterator end );

namespace simple_estimate {
double eval( Rule& rule, double factor );
} // namespace simple_estimate {

} // namespace meta_tic_tac_toe {
