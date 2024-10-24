#pragma once

#include "rule.h"

#include <random>
#include <algorithm>
#include <optional>

template< typename MoveT >
using ReOrder = std::function< void (
    GenericRule< MoveT >& rule,
    Player,
    typename std::vector< MoveT >::iterator begin,
    typename std::vector< MoveT >::iterator end) >;

template< typename MoveT >
struct Shuffle
{
    Shuffle() : g_(rd_()) {}

    void operator()( GenericRule< MoveT >&, Player,
                     typename std::vector< MoveT >::iterator begin,
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
    ReorderByScore( std::function< double (GenericRule< MoveT >&, Player) > eval )
        : eval( eval ) {}

    void operator()( GenericRule< MoveT >& rule, Player player,
                     typename std::vector< MoveT >::iterator begin,
                     typename std::vector< MoveT >::iterator end )
    {
        shuffle( rule, player, begin, end );
        scores.clear();
        for (auto itr = begin; itr != end; ++itr)
        {
            rule.apply_move( *itr, player );
            scores.push_back( std::make_pair( eval( rule, player ), *itr ));
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

    std::function< double (GenericRule< MoveT >&, Player) > eval;
    Shuffle< MoveT > shuffle;
    std::vector< std::pair< double, MoveT > > scores;
};

template< typename MoveT >
struct Negamax
{
    Negamax( GenericRule< MoveT > const& initial_rule, std::function< double (GenericRule< MoveT >&, Player) > eval,
             ReOrder< MoveT > reorder ) 
    : rule( initial_rule.clone()), eval( eval ), reorder( reorder ) {}

    std::unique_ptr< GenericRule< MoveT > > rule;
    std::function< double (GenericRule< MoveT >&, Player) > eval;
    ReOrder< MoveT > reorder;
    std::vector< MoveT > moves;

    size_t count = 0;
    size_t max_moves = 0;
    std::atomic< bool > stop = false;

    double operator()( size_t depth, Player player )
    {
        moves.clear();

        return rec( depth, player2_won, player1_won, player );
    }

    double rec( size_t depth, double alpha, double beta, Player player )
    {
        ++count;

        if (stop)
            return 0.0;

        // if we have a winner, we are done
        const Player winner = rule->get_winner();
        if (winner != not_set)
            return player * winner * player1_won;

        // save previous count of moves
        const size_t prev_size = moves.size();
        {
            auto& tmp = rule->generate_moves();
            moves.insert( moves.end(), tmp.begin(), tmp.end());
        }
        const size_t new_size = moves.size();

        // if no moves generated, we are done
        if (prev_size == new_size)
            return 0.0;

        // if max depth reached, we are done and return with a score
        if (!depth)
            return player * eval( *rule, player );

        // apply reordering of generated moves
        reorder( *rule, player, moves.begin() + prev_size, moves.end());

        double value = player2_won;
        size_t best_move = prev_size;
        size_t idx = prev_size;
        for (; idx != new_size; ++idx)
        {
            rule->apply_move( moves[idx], player );

            const double new_value =
                -rec( depth - 1, -beta, -alpha, Player( -player ));

            rule->undo_move( moves[idx], player );

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

        iter_swap( moves.begin() + prev_size, moves.begin() + best_move );

        return value;
    }
};
