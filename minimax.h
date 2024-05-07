#pragma once

#include "tree.h"

#include <functional>
#include <random>
#include <algorithm>

template< typename MoveT >
struct GenericRule
{
    virtual void reset() = 0;
    virtual void print_move( MoveT const& ) const = 0;
    virtual void print_board() const = 0;
    virtual Player get_winner() const = 0;
    virtual void generate_moves( std::vector< MoveT >& ) const = 0;
    virtual void apply_move(MoveT const& move, Player player) = 0;
    virtual void undo_move(MoveT const& move, Player) = 0;
};

template< typename MoveT >
struct Node
{
    Node(GenericRule< MoveT >& rule) : rule( rule ) {}

    void reset()
    {
        move.reset();
        rule.reset();
    }

    GenericRule< MoveT >& rule;

    std::optional< MoveT > move;
};

template< typename MoveT >
using Eval = std::function< double () >;

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
    ReorderByScore( GenericRule< MoveT >& rule, Eval< MoveT > eval )
        : rule( rule ), eval( eval ) {}

    void operator()( Player player, typename std::vector< MoveT >::iterator begin,
                     typename std::vector< MoveT >::iterator end )
    {
        shuffle( begin, end );
        scores.clear();
        for (auto itr = begin; itr != end; ++itr)
        {
            rule.apply_move( *itr, player );
            scores.push_back( std::make_pair( eval(), *itr ));
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
    Eval< MoveT > eval;
    Shuffle< MoveT > shuffle;
    std::vector< std::pair< double, MoveT > > scores;
};

struct Statistic
{
    Statistic() : nodes( 0 ), wins( 0 ), draws( 0 ), losses( 0 ), rounds( 0 ) {}

    size_t nodes;
    size_t wins;
    size_t draws;
    size_t losses;
    size_t rounds;
};

template< typename MoveT >
std::pair< double, std::optional< MoveT > > negamax(
    std::vector< MoveT >& moves, Node< MoveT > node,
    size_t depth, double alpha, double beta,
    Player player,
    Eval< MoveT > eval, ReOrder< MoveT > reorder,
    Statistic& stat,
    BuildTree< MoveT >* builder )
{
    ++stat.nodes;

    const size_t candidates_begin = moves.size();
    node.rule.generate_moves( moves );
    const size_t candidates_end = moves.size();

    const Player winner = node.rule.get_winner();
    bool is_terminal = winner != not_set
                       || candidates_begin == candidates_end;

    if (!is_terminal)
        reorder( player, moves.begin() + candidates_begin,
                 moves.begin() + candidates_end );

    std::optional< MoveT > best_move;
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
            node.move = moves[i];
            MoveT const& move = *node.move;

            node.rule.apply_move( move, player );
            if (builder)
                builder->push( move );
            const double new_value = -negamax(
                moves, node, depth - 1, -beta, -alpha, Player( -player ),
                eval, reorder, stat, builder ).first;

            if (new_value > value)
            {
                value = new_value;
                best_move = move;
            }

            node.rule.undo_move( move, player );
            if (builder)
                builder->pop();

            alpha = std::max( alpha, value );

            if (alpha >= beta)
                break;
        }
    }

    // choose some move if no best move is set
    if (!best_move && !is_terminal)
        best_move = moves[candidates_begin];

    if (builder)
        builder->update( player * value, best_move );

    // remove generated moves
    moves.resize( candidates_begin );

    return std::make_pair( value, best_move );
}
