#include "rule.h"

#include <algorithm>
#include <array>
#include <list>
#include <cmath>

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
        std::list< Vertex< MoveT > > temp_list( std::move( vertex.children ));
        assert (vertex.children.empty()); // vertex has no more children now
        swap( children, temp_list );

        return *this;
    };
    double value = 0.0;
    std::list< Vertex > children;
    MoveT move;
    bool is_terminal = false;
};

template< typename MoveT >
struct Minimax
{
    Minimax( GenericRule< MoveT >& rule,
             std::function< double (Player) > eval,
             std::function< bool (Minimax< MoveT > const&) > add_vertex_allowed )
    : rule( rule ), eval( eval ), add_vertex_allowed( add_vertex_allowed )
    {}

    Minimax( Minimax const& ) = delete;
    Minimax& operator=( Minimax const& ) = delete;

    GenericRule< MoveT >& rule;
    std::function< double (Player) > eval;
    std::function< bool (Minimax< MoveT > const&) > add_vertex_allowed;

    Vertex< MoveT > root = MoveT();
    size_t rec_count = 0;
    size_t vertex_count = 0;
    size_t depth = 0;
    size_t max_depth = 0;
    std::random_device rd;
    std::mt19937 g { rd() };

    void rec( double alpha, double beta, Player player, Vertex< MoveT >& vertex )
    {
        ++rec_count;

        if (max_depth < depth)
            max_depth = depth;

        // if we know, it's terminal, we are done
        if (vertex.is_terminal)
            return;

        // if no children generated yet (or no available)
        if (vertex.children.empty())
        {
            const Player winner = rule.get_winner();
            if (winner != not_set)
            {
                vertex.value = winner * player1_won;
                vertex.is_terminal = true; // if winner, it's terminal
                return;
            }

            auto& moves = rule.generate_moves();

            if (moves.empty())
            {
                // no winner, no moves: it's a draw and terminal
                vertex.value = 0.0;
                vertex.is_terminal = true;
                return;
            }

            // if we have to stop adding vertices, return heuristic
            if (!add_vertex_allowed(*this))
            {
                vertex.value = eval( player );
                return;
            }

            // mix in some randomness
            std::shuffle( moves.begin(), moves.end(), g );

            for (MoveT const& move : moves)
                vertex.children.emplace_back( move );
            vertex_count += moves.size();
        }

        if (player == player1)
        {
            double value = player2_won;
            for (auto itr = vertex.children.begin();
                 itr != vertex.children.end(); ++itr)
            {
                ++depth;
                rule.apply_move( itr->move, player1 );
                rec( alpha, beta, player2, *itr);
                rule.undo_move( itr->move, player1 );
                --depth;

                if (itr->value > value)
                    value = itr->value;
                if (value > alpha)
                    alpha = value;
                // prune soft-fail
                if (value >= beta)
                    break;
            }
            vertex.value = value;

            // sort descending
            vertex.children.sort(
                [](Vertex< MoveT > const& lhs, Vertex< MoveT > const& rhs)
                { return lhs.value > rhs.value; });
        }
        else // player2
        {
            double value = player1_won;
            for (auto itr = vertex.children.begin();
                 itr != vertex.children.end(); ++itr)
            {
                ++depth;
                rule.apply_move( itr->move, player2 );
                rec( alpha, beta, player1, *itr);
                rule.undo_move( itr->move, player2 );
                --depth;

                if (itr->value < value)
                    value = itr->value;
                if (value < beta)
                    beta = value;
                // prune soft-fail
                if (value <= alpha)
                   break;
            }
            vertex.value = value;

            // sort ascending
            vertex.children.sort(
                [](Vertex< MoveT > const& lhs, Vertex< MoveT > const& rhs)
                { return lhs.value < rhs.value; });
        }
    }

    void apply_move( MoveT const& move, Player player )
    {
        rule.apply_move( move, player );
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
            max_depth = 1;
            rec( player2_won, player1_won, player, root );
            depth = 0;
            /*debug
            for (auto& v : root.children)
                std::cout << int( v.move ) << ": " << v.value << ", ";
            std::cout << std::endl;*/
        } while (add_vertex_allowed( *this ));

        return root.value;
    }
};

template< typename MoveT >
struct MaxDepth
{
    MaxDepth( size_t max_depth) : max_depth( max_depth ) {}

    bool operator()( Minimax< MoveT > const& minimax )
    {
        if (minimax.depth == 0)
        {
            if (depth < max_depth)
            {
                ++depth;
                return true;
            }
            depth = 1;
            return false;
        }

        return minimax.depth < depth;
    }

    const size_t max_depth;
    size_t depth = 1;
};

template< typename MoveT >
struct MaxVertices
{
    MaxVertices( size_t max_vertices ) : max_vertices( max_vertices ) {}

    bool operator()( Minimax< MoveT > const& minimax )
    {
        const bool allowed = minimax.vertex_count < max_vertices;

        if (minimax.depth == 0)
        {
            if (allowed && depth > minimax.max_depth)
            {
                depth = 1;
                return false;
            }

            if (allowed)
                depth += 2;
            else
                depth = 1;
        }

        return allowed && minimax.depth <= depth;
    }

    const size_t max_vertices;
    size_t depth = 1;
};

template< typename MoveT >
struct ChooseFirst
{
    MoveT const& operator()(std::list< Vertex< MoveT > > const& children)
    {
        return children.front().move;
    }
};

template< typename MoveT >
struct ChooseMove
{
    ChooseMove( double epsilon ) : epsilon( epsilon ) {}

    MoveT const& operator()(std::list< Vertex< MoveT > > const& children)
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
