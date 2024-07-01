#include "rule.h"

#include <array>

namespace tic_tac_toe {

typedef u_int8_t Move;

constexpr u_int8_t n = 3;

struct Rule : public GenericRule< Move >
{
    Rule(Player*);
    virtual ~Rule() {}
    GenericRule< Move >* clone( std::vector< unsigned char >* = nullptr ) const;
    size_t size_of() const { return sizeof( Rule ); }
    void print_move( std::ostream&, Move const& ) const;
    void print_board( OutStream&, std::optional< Move > const& last_move ) const;
    Player get_winner() const;
    std::vector< Move >& generate_moves();
    void apply_move( Move const&, Player);
    void undo_move( Move const&, Player);

    Player* board;
    static std::vector< Move > moves;
};

struct DeepRule : public Rule
{
    DeepRule() : Rule( nullptr ), mem{ not_set } { board = mem.data(); }
    GenericRule< Move >* clone( std::vector< unsigned char >* = nullptr ) const;
    size_t size_of() const { return sizeof( DeepRule ); }
    std::array< Player, n * n > mem;
};

namespace trivial_estimate {
double eval( Rule const& rule );
} // namespace trivial_estimate {

namespace simple_estimate {
double eval( Rule const& rule );
} // namespace simple_estimate {

void user_input( Rule& rule,
    std::vector< Move >::iterator begin,
    std::vector< Move >::iterator end );

} // namespace tic_tac_toe {
