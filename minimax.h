#include <limits>
#include <vector>
#include <optional>
#include <functional>
#include <random>

enum Player
{
    player1 = 1,
    player2 = -1,
    not_set = 0
};

const double player1_won = std::numeric_limits< double >::infinity();
const double player2_won = -std::numeric_limits< double >::infinity();

template< typename MoveT >
struct GenericRule
{
    std::vector< MoveT > moves;

    virtual void print() = 0;
    virtual Player get_winner() const = 0;
    virtual void generate_moves() = 0;
    virtual void apply_move(MoveT const& move, Player player) = 0;
    virtual void undo_move(MoveT const& move, Player) = 0;
};

template< typename MoveT >
struct Node
{
    Node(GenericRule< MoveT >& rule) : rule( rule ) {}

    GenericRule< MoveT >& rule;

    std::optional< MoveT > move;
};

template< typename MoveT >
using EvalT = std::function< double () >;

template< typename MoveT >
using ReOrderT = std::function< void (
    typename std::vector< MoveT >::iterator begin,
    typename std::vector< MoveT >::iterator end) >;

template< typename MoveT >
struct Shuffle
{
    Shuffle() : g_(rd_()) {}

    void operator()( typename std::vector< MoveT >::iterator begin,
                     typename std::vector< MoveT >::iterator end)
    {
        std::shuffle( begin, end, g_ );
    }

    std::random_device rd_;
    std::mt19937 g_;
};

struct Statistic
{
    Statistic() : count_( 0 ) {}

    void operator()()
    {
        ++count_;
    }

    size_t count_;
};

template< typename BuildTreeT, typename MoveT >
std::pair< double, std::optional< size_t > > negamax(
    Node< MoveT > node, size_t depth, double alpha, double beta,
    Player player,
    EvalT< MoveT > eval, ReOrderT< MoveT >& reorder,
    BuildTreeT builder )
{
    const size_t candidates_begin = node.rule.moves.size();
    node.rule.generate_moves();
    const size_t candidates_end = node.rule.moves.size();

    const Player winner = node.rule.get_winner();
    bool is_terminal = winner != not_set
                       || candidates_begin == candidates_end;

    if (!is_terminal)
        reorder( node.rule.moves.begin() + candidates_begin,
                 node.rule.moves.begin() + candidates_end );

    std::optional< size_t > best_move;
    double value;

    if (is_terminal && winner != not_set)
        value = player * winner * player1_won;
    else if (is_terminal || !depth)
        value = player * eval();
    else
    {
        value = player2_won;
        for (size_t i = candidates_begin; i != candidates_end; ++i)
        {
            node.move = node.rule.moves[i];

            node.rule.apply_move( *node.move, player );

            const double new_value = -negamax(
                node, depth - 1, -beta, -alpha, Player( -player ),
                eval, reorder, builder( *node.move )).first;

            if (new_value > value)
            {
                value = new_value;
                best_move = node.move;
            }

            node.rule.undo_move( *node.move, player );

            alpha = std::max( alpha, value );

            if (alpha >= beta)
                break;
        }
    }

    // choose some move if no best move is set
    if (!best_move && !is_terminal)
        best_move = node.rule.moves[candidates_begin];

    builder.update( player * value, best_move );

    // remove generated moves
    node.rule.moves.resize( candidates_begin );

    return std::make_pair( value, best_move );
}
