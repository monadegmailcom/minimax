#pragma once

#include "player.h"

#include <string>
#include <optional>
#include <vector>

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
    virtual ~GenericRule() {}
    virtual GenericRule* clone() const = 0;
    virtual void copy_from( GenericRule const& ) = 0;
    virtual void print_move( std::ostream&, MoveT const& ) const = 0;
    virtual void print_board( OutStream&, std::optional< MoveT > const& last_move ) const = 0;
    virtual Player get_winner() const = 0;
    virtual std::vector< MoveT >& generate_moves() const = 0;
    virtual void apply_move(MoveT const& move, Player player) = 0;
    virtual void undo_move(MoveT const& move, Player) = 0;
};