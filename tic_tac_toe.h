#include "minimax.h"

template< typename T >
Player extract_player( T const& v );

namespace tic_tac_toe {

typedef size_t Move;

constexpr size_t n = 3;

struct Rule : public GenericRule< Move >
{
    Rule(Player*);
    void reset();
    void print_move( Move const& ) const;
    void print_board() const;
    Player get_winner() const;
    void generate_moves( std::vector< Move >& ) const;
    void apply_move( Move const& move, Player player);
    void undo_move( Move const& move, Player);

    Player* const board;
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
