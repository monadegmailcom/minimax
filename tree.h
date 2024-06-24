#include "negamax.h"
#include "minimax.h"

#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <limits>
#include <iomanip>
#include <cassert>

void print_preamble( std::ostream& );
void print_closing( std::ostream& );

enum DisplayNode { Board = 1, Stats = -1 };

template< typename MoveT >
struct Node
{
    Node( Vertex< MoveT > const& vertex ) : size( 1 ), vertex( vertex ), depth( 1 )
    {
        for (Vertex< MoveT > const& child_vertex : vertex.children)
        {
            children.emplace_back( child_vertex );
            Node const& child_node = children.back();
            size += child_node.size;
            if (child_node.depth > depth)
                depth = child_node.depth;
        }
        if (!children.empty())
            ++depth;
    }
    std::list< Node > children;

    size_t size;
    Vertex< MoveT > const& vertex;
    size_t depth;
};

template< typename MoveT >
struct PrintTree
{
    PrintTree( std::ostream& stream, Node< MoveT > const& node,
               GenericRule< MoveT >& rule, Player player,
               DisplayNode display_node = Board,
               Node< MoveT > const* emph = nullptr,
               size_t depth = std::numeric_limits< size_t >::max()) :
        stream( stream ), rule( rule ), emph( emph ),
        out_stream( OutStream { stream, "<B>", "</B>", "<BR/>", "&nbsp;" } ),
        display_node( display_node )
    {
        print_preamble( stream );
        rec( "v", node, nullptr, player, depth );
        print_closing( stream );
    }

    void rec( std::string const& name, Node< MoveT > const& node,
              Node< MoveT > const* parent, Player player, size_t depth )
    {
        // print vertex
        stream << "\"" << name << "\" [label=<";
        if (display_node == Board)
            rule.print_board( out_stream, node.vertex.move );
        else
        {
            stream << std::fixed << std::setprecision( 2 )
                   << "v = " << std::setw( 5 ) << node.vertex.value << "<BR/>"
                   << "d = " << std::setw( 5 ) << node.depth << "<BR/>"
                   << "s = " << std::setw( 5 ) << node.size << "<BR/>"
                   << "r = " << std::setw( 5 ) << (parent ? double( node.size ) / (parent->size - 1) : 1 );
        }
        stream << ">";
        if (&node == emph)
            stream << " color=\"red\"";
        if (player == player1)
            stream << " shape=box";
        stream << "]\n";

        --depth;
        if (!depth)
            return;

        for (Node< MoveT > const& child : node.children)
        {
            std::ostringstream s;
            rule.print_move( s, child.vertex.move );
            const std::string new_name = name + " " + s.str();

            // plot edge to child
            stream << "\"" << name << "\"" << " -- \"" << new_name << "\"\n";

            // apply move
            rule.apply_move( child.vertex.move, player );

            rec( new_name, child, &node, Player( -player ), depth);
            // undo move
            rule.undo_move( child.vertex.move, player );
        }
    }

    std::ostream& stream;
    GenericRule< MoveT >& rule;
    Node< MoveT > const* emph;
    OutStream out_stream;
    DisplayNode display_node;
};

template< typename MoveT >
void tree_lens( GenericRule< MoveT >& initial_rule, Node< MoveT > const& root, Player player )
{
    size_t depth = 2;
    DisplayNode display_node = Stats;

    std::vector< Node< MoveT > const* > path;
    path.push_back( &root );
    Node< MoveT > const* node =
        root.children.empty() ? nullptr : &root.children.front();

    std::unique_ptr< GenericRule< MoveT > > rule( initial_rule.clone());

    const std::string filename = "lens.gv";
    while (true)
    {
        {
            std::ofstream gv( filename );
            PrintTree< MoveT > print_tree(
                gv, *path.back(), *rule, player,
                display_node, node, depth );
        }

        char c;
        std::cout << "your choice (q, t, +, -, u, d, l, r)?";
        std::cin >> c;
        if (c == 'q')
            break;
        else if (c == 't')
            display_node = DisplayNode( -display_node );
        else if (c == '+')
        {
            ++depth;
            std::cout << "set depth to " << depth << std::endl;
        }
        else if (c == '-' && depth > 1)
        {
            --depth;
            std::cout << "set depth to " << depth << std::endl;
        }
        else if (c == 'u') // up
        {
            if (path.size() >= 2)
            {
                node = path.back();
                path.pop_back();
                rule->undo_move( node->vertex.move, player );
                player = Player( -player );
            }
        }
        else if (c == 'd') // down
        {
            if (!node->children.empty())
            {
                path.push_back( node );
                rule->apply_move( node->vertex.move, player );
                node = &node->children.front();
                player = Player( -player );
            }
        }
        else if (c == 'l') // left
        {
            std::list< Node< MoveT > > const& children = path.back()->children;
            auto itr = find_if(
                children.begin(), children.end(),
                [node](Node< MoveT > const& v)
                {return &v == node;});
            if (itr != children.end() && itr != children.begin())
                node = &*(--itr);
        }
        else if (c == 'r') // right
        {
            std::list< Node< MoveT > > const& children = path.back()->children;
            auto itr = find_if(
                children.rbegin(), children.rend(),
                [node](Node< MoveT > const& v)
                {return &v == node;});
            if (itr != children.rend() && itr != children.rbegin())
                node = &*(--itr);
        }
    }
}
