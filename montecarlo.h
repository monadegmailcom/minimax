#include "rule.h"

#include <boost/pool/pool_alloc.hpp>

#include <list>
#include <memory>
#include <optional>
#include <cmath>

namespace montecarlo {

template< typename MoveT >
struct Node;

template< typename MoveT >
using NodeList = std::list<
    Node< MoveT >,
    boost::fast_pool_allocator< Node< MoveT >,
    boost::default_user_allocator_new_delete,
    boost::details::pool::null_mutex > >;

template< typename MoveT >
struct Node
{
    Node( MoveT const& move ) : move( move ) {}
    Node( Node const& ) = delete;
    Node& operator=( Node const& ) = delete;
    Node& operator=( MoveT const& m)
    {
        move = m;
        numerator = 0.0;
        denominator = 0.0;
        is_terminal.reset();
        children.clear();
        return *this;
    }

    Node& operator=( Node&& node )
    {
        move = node.move;
        numerator = node.numerator;
        denominator = node.denominator;
        is_terminal = node.is_terminal;

        // no copies or assignments by moving
        NodeList< MoveT > temp_list( std::move( node.children ));
        assert (node.children.empty());
        swap( children, temp_list );

        return *this;
    };

    MoveT move;
    double numerator = 0.0;
    size_t denominator = 0;
    std::optional< Player > is_terminal;
    NodeList< MoveT > children;
};

template< typename MoveT >
struct MCTS
{
    MCTS( double exploration ) : exploration( exploration ), gen( rd())
    {}

    Node< MoveT >& select( Node< MoveT >& node )
    {
        assert (!node.children.empty());

        values.clear();
        for (Node< MoveT >& child : node.children)
            values.push_back( { cbt( child, node, exploration ), &child });

        // mix in some randomness
        std::shuffle( values.begin(), values.end(), gen );

        auto itr = max_element( values.begin(), values.end(),
            [](auto& lhs, auto& rhs) { return lhs.first < rhs.first; });
        return *itr->second;
    }

    static double cbt(
        Node< MoveT > const& child, Node< MoveT > const& node,
        double exploration )
    {
        return !child.denominator
            ? INFINITY
            : 1 - child.numerator / child.denominator
                + exploration * sqrt( log2( node.denominator ) / child.denominator);
    }

    Player playout( Node< MoveT >& node, Player player)
    {
        assert (node.children.empty());

        std::unique_ptr< GenericRule< MoveT >, NoDelete< MoveT > >
            local_rule( rule->clone( &rule_buf ));

        Player winner;
        bool first = true;
        while (true)
        {
            winner = local_rule->get_winner();
            if (winner != not_set) // win
                break;

            auto& moves = local_rule->generate_moves();
            if (moves.empty()) // draw
                break;

            // expansion
            if (first)
                for (MoveT const& move : moves)
                    node.children.emplace_back( move );

            // apply new random move
            auto idx = std::uniform_int_distribution<std::mt19937::result_type>(
                0, moves.size() - 1)( rd );
            local_rule->apply_move( moves[idx], player );

            player = Player( -player );
            first = false;
        }

        if (first)
            node.is_terminal = winner;

        // debug
        //std::cout << "winner = " << winner << std::endl;
        return winner;
    }

    Player simulate( Node< MoveT >& node, Player player )
    {
        if (node.is_terminal)
            return *node.is_terminal;

        Player winner;
        if (node.children.empty())
            winner = playout( node, player );
        else
        {
            Node< MoveT >& child = select( node );
            rule->apply_move( child.move, player );
            winner = simulate( child, Player( -player ));
            rule->undo_move( child.move, player );
        }

        // back propagation
        ++node.denominator;
        if (winner == player)
            node.numerator += 1.0;
        else if (winner == not_set)
            node.numerator += 0.5;

        return winner;
    }

    void operator()( size_t simulations, Player player )
    {
        for(; simulations && !root.is_terminal; --simulations)
        {
            //debug( this );
            simulate( root, player );
        }
    }

    void apply_move( MoveT const& move, Player player )
    {
        rule->apply_move( move, player );
        auto itr = find_if( root.children.begin(), root.children.end(),
            [&move]( Node< MoveT > const& node) { return node.move == move; });
        if (itr != root.children.end())
            root = std::move( *itr );
        else
            root = move;
    }

    void init(GenericRule< MoveT >* r)
    {
        rule = r;
        rule_buf.resize( rule->size_of());
        root = { MoveT() };
    }

    GenericRule< MoveT >* rule = nullptr;
    double exploration;
    Node< MoveT > root = { MoveT() };
    std::vector< std::pair< double, Node< MoveT >* > > values;
    std::vector< unsigned char > rule_buf;
    std::random_device rd;
    std::mt19937 gen;
    std::function< void (MCTS*) > debug;
};

} // namespace montecarlo {
