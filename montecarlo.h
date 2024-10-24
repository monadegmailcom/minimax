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
    /*Node( Node const& node ) : move( node.move ), numerator( node.numerator ),
        denominator( node.denominator ), is_terminal( node.is_terminal ), children( node.children )
    {}*/

    Node& operator=( Node const& ) = delete;
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
    MCTS( GenericRule< MoveT > const& initial_rule, double exploration )
    : rule( initial_rule.clone()), playout_rule( initial_rule.clone()), exploration( exploration ), gen( rd())
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
        Node< MoveT > const& child, Node< MoveT > const& node, double exploration )
    {
        return !child.denominator
            ? INFINITY
            : 1 - child.numerator / child.denominator
                + exploration * sqrt( log( node.denominator ) / child.denominator);
    }

    Player playout( std::vector< MoveT >& moves, Player player)
    {
        assert( !moves.empty());
        
        playout_rule->copy_from( *rule );

        Player winner;
        while (true)
        {
            // apply new random move
            auto idx = std::uniform_int_distribution<std::mt19937::result_type>(
                0, moves.size() - 1)( rd );
            playout_rule->apply_move( moves[idx], player );
            winner = playout_rule->get_winner();

            if (winner != not_set) // win?
                break;

            // generate_moves updates the static moves container
            playout_rule->generate_moves();
            if (moves.empty()) // draw?
                break;

            player = Player( -player );
        }

        return winner;
    }

    Player simulate( Node< MoveT >& node, Player player )
    {
        Player winner;
        if (node.is_terminal)
            winner = *node.is_terminal;
        else if (node.children.empty())
        {
            winner = rule->get_winner();
            if (winner != not_set) // winner?
                node.is_terminal = winner;
            else 
            {
                auto& moves = rule->generate_moves();
                if (moves.empty()) // draw?
                    node.is_terminal = not_set;
                else
                {
                    // expansion
                    for (MoveT const& move : moves)
                        node.children.emplace_back( move );
            
                    winner = playout( moves, player );
                    // debug
                    //std::cout << "winner of move " << int( node.move ) << " = " << winner << std::endl;
                }
            }
        }
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
            if (stop)
                return;
            simulate( root, player );
            //debug( this );
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

    void init(GenericRule< MoveT > const& r)
    {
        rule->copy_from( r );
        root = Node< MoveT >( MoveT()); // { MoveT() }; // todo
    }

    std::unique_ptr< GenericRule< MoveT > > rule;
    std::unique_ptr< GenericRule< MoveT > > playout_rule;
    double exploration;
    Node< MoveT > root = { MoveT() };
    std::vector< std::pair< double, Node< MoveT >* > > values;
    std::random_device rd;
    std::mt19937 gen;
    std::atomic< bool > stop = false;
};

} // namespace montecarlo {
