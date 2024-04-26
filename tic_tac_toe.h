#include "minimax.h"

namespace tic_tac_toe {

struct Rule : public GenericRule< size_t >
{
    typedef size_t Move;

    Rule( size_t n );
    void print() const;
    Player get_winner() const;
    void generate_moves();
    void apply_move( Move const& move, Player player);
    void undo_move( Move const& move, Player);

    size_t n;
    std::vector< Player > board;
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

void print_tree( std::ostream& stream, TreeNode< size_t > const& root,
                 Rule& rule );

} // namespace tic_tac_toe {
