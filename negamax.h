#pragma once

#include <functional>
#include <random>
#include <algorithm>
#include <optional>

#include "player.h"

template< typename MoveT >
struct Vertex
{
    Vertex( std::optional< MoveT > const& move )
        : move( move ), value( 0.0 ), child_count( 0 ) {}
    std::optional< MoveT > move;
    double value;
    size_t child_count;
    std::optional< MoveT > best_move;
};

struct OutStream
{
    std::ostream& stream;
    std::string emph_start;
    std::string emph_end;
    std::string linebreak;
    std::string space;
};

template< typename MoveT >
struct GenericRule
{
    virtual void reset() = 0;
    virtual void snapshot() = 0;
    virtual void restore_snapshot() = 0;
    virtual void print_move( std::ostream&, MoveT const& ) const = 0;
    virtual void print_board( OutStream&, std::optional< MoveT > const& last_move ) const = 0;
    virtual Player get_winner() const = 0;
    virtual void generate_moves( std::vector< MoveT >& ) const = 0;
    virtual void apply_move(MoveT const& move, Player player) = 0;
    virtual void undo_move(MoveT const& move, Player) = 0;
};

using Eval = std::function< double (Player) >;

template< typename MoveT >
using ReOrder = std::function< void (
    Player,
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

template< typename MoveT >
struct ReorderByScore
{
    ReorderByScore( GenericRule< MoveT >& rule, Eval eval )
        : rule( rule ), eval( eval ) {}

    void operator()( Player player, typename std::vector< MoveT >::iterator begin,
                     typename std::vector< MoveT >::iterator end )
    {
        shuffle( begin, end );
        scores.clear();
        for (auto itr = begin; itr != end; ++itr)
        {
            rule.apply_move( *itr, player );
            scores.push_back( std::make_pair( eval( player ), *itr ));
            rule.undo_move( *itr, player );
        }

        std::function< bool (std::pair< double, MoveT >,
                             std::pair< double, MoveT >) > pred;
        if (player == player1)
            pred = [](auto lhs, auto rhs) { return lhs.first > rhs.first; };
        else
            pred = [](auto lhs, auto rhs) { return lhs.first < rhs.first; };
        sort( scores.begin(), scores.end(), pred );

        auto itr2 = scores.begin();
        for (auto itr = begin; itr != end; ++itr, ++itr2)
            *itr = itr2->second;
    }

    GenericRule< MoveT >& rule;
    Eval eval;
    Shuffle< MoveT > shuffle;
    std::vector< std::pair< double, MoveT > > scores;
};

template< typename MoveT >
struct Minimax
{
    Minimax( GenericRule< MoveT >& rule, bool trace ) : rule( rule ), trace( trace )
    {}

    GenericRule< MoveT >& rule;
    bool trace;
    Eval eval;
    ReOrder< MoveT > reorder;
    std::vector< MoveT > moves;
    std::vector< Vertex< MoveT > > vertices;

    static size_t count;
    static size_t max_moves;

    double operator()( size_t depth, Player player )
    {
        moves.clear();
        vertices.clear();

        vertices.push_back( Vertex< MoveT >( std::optional< MoveT >()));

        return rec( depth, player2_won, player1_won, player );
    }

    double rec( size_t depth, double alpha, double beta, Player player )
    {
        ++count;

        // if we have a winner, we are done
        const Player winner = rule.get_winner();
        if (winner != not_set)
            return player * winner * player1_won;

        // save previous count of moves
        const size_t prev_size = moves.size();
        rule.generate_moves( moves );
        const size_t new_size = moves.size();

        // if no moves generated, we are done
        if (prev_size == new_size)
            return 0.0;

        // apply reordering of generated moves
        reorder( player, moves.begin() + prev_size, moves.end());

        // if max depth reached, we are done and return with a score
        if (!depth)
            return player * eval( player );

        double value = player2_won;
        size_t best_move = prev_size;
        size_t idx = prev_size;
        const size_t prev_vertex_idx = vertices.size() - 1;
        size_t vertex_count = 0;
        for (; idx != new_size; ++idx)
        {
            rule.apply_move( moves[idx], player );

            const size_t vertex_idx = vertices.size();
            if (trace)
            {
                ++vertex_count;
                vertices.push_back( Vertex< MoveT >( moves[idx] ));
            }

            const double new_value =
                -rec( depth - 1, -beta, -alpha, Player( -player ));

            if (trace)
                vertices[vertex_idx].value = player * new_value;

            rule.undo_move( moves[idx], player );

            if (moves.size() > max_moves)
                max_moves = moves.size();

            moves.resize( new_size );

            if (new_value > value)
            {
                value = new_value;
                best_move = idx;
            }

            alpha = std::max( alpha, value );

            if (alpha >= beta)
                break;
        }

        if (trace)
        {
            Vertex< MoveT >& vertex = vertices[prev_vertex_idx];
            vertex.child_count = vertex_count;
            vertex.best_move = moves[best_move];
        }

        iter_swap( moves.begin() + prev_size, moves.begin() + best_move );

        return value;
    }
};

template< typename MoveT > size_t Minimax< MoveT >::count = 0;
template< typename MoveT > size_t Minimax< MoveT >::max_moves = 0;
