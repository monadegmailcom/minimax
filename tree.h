#include "player.h"

#include <vector>
#include <memory>
#include <optional>

template< typename MoveT >
struct TreeNode
{
    TreeNode( Player player, std::optional< MoveT > move,
              std::shared_ptr< TreeNode > parent )
        : player_( player ), move_( move ), parent_( parent ) {}
    Player player_;
    double value_;
    std::optional< MoveT > move_;
    std::optional< MoveT > best_move_;
    std::vector< std::shared_ptr< TreeNode > > children_;
    std::shared_ptr< TreeNode > parent_;
};

template< typename MoveT >
struct BuildTree
{
    BuildTree( Player player )
        : root_( std::make_shared< TreeNode< MoveT > >(
                    player, std::optional< MoveT >(),
                    std::shared_ptr< TreeNode< MoveT > >())) {}
    void push( MoveT const& move )
    {
        auto new_node( std::make_shared< TreeNode< MoveT > >(
                Player (-root_->player_ ), move, root_ ));
        root_->children_.push_back( new_node );
        root_ = new_node;
    }

    void pop()
    {
        root_ = root_->parent_;
    }

    void update( double value, std::optional< size_t > best_move )
    {
        root_->value_ = value;
        root_->best_move_ = best_move;
    }

    std::shared_ptr< TreeNode< MoveT > > root_;
};

template< typename MoveT >
using PrintTree = std::function< void (TreeNode< MoveT > const&) >;
