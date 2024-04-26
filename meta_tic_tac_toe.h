#include "tic_tac_toe.h"

namespace meta_tic_tac_toe {

struct Rule : public GenericRule< std::pair< size_t, size_t > >
{
    typedef std::pair< size_t, size_t > Move;

    Rule( size_t inner_n, size_t n );
    void print() const;
    Player get_winner() const;
    void generate_moves();
    void apply_move( Move const& move, Player player);
    void undo_move( Move const& move, Player);

    size_t n;
    size_t inner_n;
    std::vector< std::pair< tic_tac_toe::Rule, Player > > board;
};

} // namespace meta_tic_tac_toe {
