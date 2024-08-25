#include "rule.h"

#include <algorithm>
#include <array>
#include <list>
#include <cmath>
#include <boost/pool/pool_alloc.hpp>

template< typename MoveT >
struct Vertex;

template< typename MoveT >
using VertexList = std::list<
    Vertex< MoveT >,
    boost::fast_pool_allocator< Vertex< MoveT >,
    boost::default_user_allocator_new_delete,
    boost::details::pool::null_mutex > >;

template< typename MoveT >
struct Vertex
{
    Vertex( MoveT const& move ) : move( move ) {}
    Vertex( Vertex const& ) = delete;
    Vertex& operator=( Vertex const& vertex ) = delete;
    Vertex& operator=( Vertex&& vertex )
    {
        move = vertex.move;
        value = vertex.value;
        is_terminal = vertex.is_terminal;

        // no copies or assignments by moving
        VertexList< MoveT > temp_list( std::move( vertex.children ));
        assert (vertex.children.empty()); // vertex has no more children now
        swap( children, temp_list );

        return *this;
    };

    double value = 0.0;
    VertexList< MoveT > children;
    MoveT move;
    bool is_terminal = false;
};

template< typename MoveT >
struct Minimax;

enum RecState { Continue, SoftStop, HardStop };

template< typename MoveT >
struct Recursion
{
    virtual RecState operator()( Minimax< MoveT > const& ) = 0;
};

template< typename MoveT >
struct Minimax
{
    Minimax( GenericRule< MoveT > const& initial_rule, std::function< double (GenericRule< MoveT >&, Player) > eval,
             Recursion< MoveT >& recursion )
    : rule( initial_rule.clone()), eval( eval ), recursion( recursion )
    {}

    Minimax( Minimax const& ) = delete;
    Minimax& operator=( Minimax const& ) = delete;

    std::unique_ptr< GenericRule< MoveT > > rule;
    std::function< double (GenericRule< MoveT >&, Player) > eval;
    Recursion< MoveT >& recursion;

    Vertex< MoveT > root = MoveT();
    size_t rec_count = 0;
    size_t vertex_count = 0;
    size_t depth = 0;
    std::random_device rd;
    std::mt19937 g { rd() };
    std::function< void (Minimax*) > debug;

    static bool prune1( double& alpha, double& beta, double& value,
                        Vertex< MoveT >& vertex )
    {
        if (vertex.value > value)
            value = vertex.value;
        if (value > alpha)
            alpha = value;
        // prune soft-fail
        return (value >= beta);
    }

    static bool pred1( Vertex< MoveT > const& lhs, Vertex< MoveT > const& rhs)
    {
        return lhs.value > rhs.value;
    }

    static bool prune2( double& alpha, double& beta, double& value,
                        Vertex< MoveT >& vertex )
    {
        if (vertex.value < value)
            value = vertex.value;
        if (value < beta)
            beta = value;
        // prune soft-fail
        return (value <= alpha);
    }

    static bool pred2( Vertex< MoveT > const& lhs, Vertex< MoveT > const& rhs)
    {
        return lhs.value < rhs.value;
    }

    // return true if hard stop
    bool rec( double alpha, double beta, Player player, Vertex< MoveT >& vertex )
    {
        ++rec_count;

        // if we know, it's terminal, we are done
        if (vertex.is_terminal)
            return false;

        // if no children generated yet (or no available)
        if (vertex.children.empty())
        {
            const Player winner = rule->get_winner();
            if (winner != not_set)
            {
                vertex.value = winner * player1_won;
                vertex.is_terminal = true; // if winner, it's terminal
                return false;
            }

            auto& moves = rule->generate_moves();

            if (moves.empty())
            {
                // no winner, no moves: it's a draw and terminal
                vertex.value = 0.0;
                vertex.is_terminal = true;
                return false;
            }

            // if we have to stop adding vertices, return heuristic
            const RecState rec_state = recursion(*this);
            if (rec_state == SoftStop)
            {
                vertex.value = eval( *rule, player );
                return false;
            }
            else if (rec_state == HardStop)
                return true;

            // mix in some randomness
            std::shuffle( moves.begin(), moves.end(), g );

            for (MoveT const& move : moves)
                vertex.children.emplace_back( move );
            vertex_count += moves.size();
        }

        double value;
        bool (*prune)( double&, double&, double&, Vertex< MoveT >& );
        bool (*pred)(Vertex< MoveT > const&, Vertex< MoveT > const& );

        if (player == player1)
        {
            prune = &Minimax< MoveT >::prune1;
            pred = &Minimax< MoveT >::pred1;
            value = player2_won;
        }
        else
        {
            prune = &Minimax< MoveT >::prune2;
            pred = &Minimax< MoveT >::pred2;
            value = player1_won;
        }

        bool is_terminal = true;

        for (auto itr = vertex.children.begin();
             itr != vertex.children.end(); ++itr)
        {
            ++depth;
            rule->apply_move( itr->move, player );
            const bool hard_stop = rec( alpha, beta, Player( -player ), *itr);
            rule->undo_move( itr->move, player1 );
            --depth;

            if (hard_stop)
                return true;

            is_terminal &= itr->is_terminal;

            if (prune( alpha, beta, value, *itr ))
                break;
        }

        vertex.value = value;
        vertex.is_terminal = is_terminal;
        vertex.children.sort( pred );

        return false;
    }

    void apply_move( MoveT const& move, Player player )
    {
        rule->apply_move( move, player );
        auto itr = find_if( root.children.begin(), root.children.end(),
            [&move]( Vertex< MoveT > const& vertex) { return vertex.move == move; });
        if (itr != root.children.end())
            root = std::move( *itr );
        else
            root = Vertex< MoveT >( move );
    }

    double operator()( Player player )
    {
        rec_count = 0;
        vertex_count = 0;

        do
        {
            depth = 1;
            rec( player2_won, player1_won, player, root );
            depth = 0;
            //debug( this );

            /*debug
            for (auto& v : root.children)
                std::cout << int( v.move ) << ": " << v.value << ", ";
            std::cout << std::endl;*/
        } while (recursion( *this ) == Continue);

        return root.value;
    }
};

template< typename MoveT >
struct MaxDepth : public Recursion< MoveT >
{
    MaxDepth( size_t max_depth) : max_depth( max_depth ) {}

    RecState operator()( Minimax< MoveT > const& minimax )
    {
        if (minimax.depth == 0)
        {
            if (depth < max_depth)
            {
                ++depth;
                // debug
                //std::cout << "depth = " << depth << std::endl;

                return Continue;
            }
            if (depth > 2)
                depth -= 2;
            else
                depth = 1;
            return SoftStop;
        }

        return minimax.depth < depth ? Continue : SoftStop;
    }

    const size_t max_depth;
    size_t depth = 1;
};

template< typename MoveT >
struct MaxVertices : public Recursion< MoveT >
{
    MaxVertices( size_t max_vertices ) : max_vertices( max_vertices ) {}

    RecState operator()( Minimax< MoveT > const& minimax )
    {
        const bool allowed = minimax.vertex_count < max_vertices;

        if (minimax.depth == 0)
        {
            if (!allowed || minimax.root.is_terminal)
            {
                if (depth > 2)
                    depth -= 2;
                else
                    depth = 1;
                return SoftStop;
            }

            ++depth;

            // debug
            //std::cout << "depth = " << depth << std::endl;

            return Continue;
        }

        if (!allowed)
            return HardStop;
        else if (minimax.depth > depth)
            return SoftStop;
        else
            return Continue;
    }

    const size_t max_vertices;
    size_t depth = 1;
};

template< typename MoveT >
struct ChooseFirst
{
    MoveT const& operator()(VertexList< MoveT > const& children)
    {
        return children.front().move;
    }
};

template< typename MoveT >
struct ChooseMove
{
    ChooseMove( double epsilon ) : epsilon( epsilon ) {}

    MoveT const& operator()(VertexList< MoveT > const& children)
    {
        const double v = children.front().value ? *children.front().value : 0.0;
        moves.clear();
        for (auto itr = children.begin();
             itr != children.end() && itr->value
             && (std::isnan( *itr->value - v)
                 || std::abs( *itr->value - v ) <= epsilon); ++itr)
            moves.push_back( itr->move );
        auto dist = std::uniform_int_distribution< int >( 0, moves.size() - 1);
        return moves[dist( g )];
    }

    const double epsilon;
    std::vector< MoveT > moves;
    std::random_device rd;
    std::mt19937 g { rd() };
};
