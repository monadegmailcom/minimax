#include "negamax.h"
#include "minimax.h"
#include "montecarlo.h"

#include <graphviz/gvc.h>
#include <librsvg/rsvg.h>

#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <limits>
#include <iomanip>
#include <cassert>
/* 
void print_preamble( std::ostream&, TreeType );
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
               TreeType type = Hierarchie,
               DisplayNode display_node = Board,
               Node< MoveT > const* emph = nullptr,
               size_t depth = std::numeric_limits< size_t >::max()) :
        stream( stream ), rule( rule ), emph( emph ),
        out_stream( OutStream { stream, "<B>", "</B>", "<BR/>", "&nbsp;" } ),
        display_node( display_node )
    {
        print_preamble( stream, type );
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
                   << "m = " << std::setw( 5 );
            rule.print_move(stream, node.vertex.move);
            stream << "<BR/>"
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
void tree_lens( GenericRule< MoveT > const& initial_rule, 
                Node< MoveT > const& root, Player player )
{
    size_t depth = 2;
    DisplayNode display_node = Stats;
    TreeType tree_type = Hierarchie;
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
                tree_type, display_node, node, depth );
        }

        char c;
        std::cout << "your choice (q, t, c, +, -, u, d, l, r)?";
        std::cin >> c;
        if (c == 'q')
            break;
        else if (c == 't')
            display_node = DisplayNode( -display_node );
        else if (c == 'c')
            tree_type = TreeType( -tree_type );
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
 */


enum DisplayNode { Board, SingleMove, Stats };
enum Layout { Hierarchie, Circular };

class GraphvizTree
{
public:
    GraphvizTree( GVC_t* gv_gvc, Player player );

    virtual ~GraphvizTree();

    virtual void set_node_attribute( Agnode_t* gv_node ) = 0;

    std::pair< char*, unsigned > render( DisplayNode _display_node, Layout layout );
    Agraph_t* get_graph() { return gv_graph; }

    void create_subgraph( size_t depth );
    void set_focus_node( Agnode_t* node ) { gv_focus_node = node; }
    pointf get_focus_coord() const;
protected:
    std::ostringstream value; // reuse allocated memory
    DisplayNode display_node = Stats;
    Agraph_t* gv_graph = nullptr;
    Agraph_t* gv_subgraph = nullptr;
private:
    GVC_t* gv_gvc = nullptr;
    Agnode_t* gv_focus_node = nullptr;
    void add_node_to_subgraph( Agnode_t* gv_node, size_t depth );
};

namespace montecarlo
{

class Tree : public GraphvizTree
{
public:
    Tree( GVC_t* gv_gvc, Player player, float exploration );

    struct Data
    {
        Agrec_t h;
        size_t depth;
        size_t size;
        char* move;
        double numerator;
        size_t denominator;
        double cbt;
    };
    float get_exploration() const { return exploration; }
private:
    float exploration;

    void set_node_attribute( Agnode_t* gv_node );
};

template< typename MoveT >
Agnode_t* add_node( 
    GenericRule< MoveT > const& rule, Tree& tree, Player player, 
    montecarlo::Node< MoveT > const& node )
{
    Agnode_t* gv_node = agnode(tree.get_graph(), nullptr, true);

    Tree::Data* data = (Tree::Data*) agbindrec( gv_node, "data", sizeof(Tree::Data), false);
    data->depth = 1;
    data->size = 1;
    static std::ostringstream value; // reuse allocated memory
    value.str( "");
    rule.print_move( value, node.move );
    data->move = agstrdup( tree.get_graph(), value.str().data());
    data->numerator = node.numerator;
    data->denominator = node.denominator;
    data->cbt = 0.0;

    for (montecarlo::Node< MoveT > const& child : node.children)
    {
        Agnode_t* gv_child = add_node( rule, tree, Player( -player ), child );
        agedge(tree.get_graph(), gv_node, gv_child, nullptr, true);

        Tree::Data* child_data = (Tree::Data*)aggetrec(gv_child, "data", 0);
        child_data->cbt = MCTS< MoveT >::cbt( child, node, tree.get_exploration());
        data->size += child_data->size;
        if (child_data->depth > data->depth)
            data->depth = child_data->depth;
    }
    if (!node.children.empty())
        ++data->depth;

    return gv_node;
}

template< typename MoveT >
std::unique_ptr< GraphvizTree > build_tree( 
    GVC_t* gv_gvc, 
    GenericRule< MoveT >& rule, 
    Player player, 
    float exploration, 
    montecarlo::Node< MoveT > const& node )
{
    auto tree = std::make_unique< Tree >( gv_gvc, player, exploration );
    add_node( rule, *tree, player, node );
    tree->set_focus_node( agfstnode( tree->get_graph()));

    return tree;
}

/*
template< typename MoveT >
struct PrintTree
{
    PrintTree( std::ostream& stream, Node< MoveT > const& node,
               double exploration,
               GenericRule< MoveT >& rule, Player player,
               TreeType type = Hierarchie,
               DisplayNode display_node = Board,
               Node< MoveT > const* emph = nullptr,
               size_t depth = std::numeric_limits< size_t >::max()) :
        stream( stream ), rule( rule ), exploration( exploration ),
        emph( emph ),
        out_stream( OutStream { stream, "<B>", "</B>", "<BR/>", "&nbsp;" } ),
        display_node( display_node )
    {
        print_preamble( stream, type );
        rec( "v", node, nullptr, player, depth );
        print_closing( stream );
    }

    void rec( std::string const& name, Node< MoveT > const& node,
              Node< MoveT > const* parent, Player player, size_t depth )
    {
        // print vertex
        stream << "\"" << name << "\" [label=<";
        if (display_node == Board)
            rule.print_board( out_stream, node.node.move );
        else
        {
            stream << std::fixed << std::setprecision( 2 )
                   << "m = " << std::setw( 5 );
            rule.print_move(stream, node.node.move);
            stream << "<BR/>"
                   << "w = " << std::setw( 5 ) << node.node.numerator << "<BR/>"
                   << "n = " << std::setw( 5 ) << node.node.denominator << "<BR/>"
                   << "v = " << std::setw( 5 );
            if (parent)
                stream << MCTS< MoveT >::cbt( node.node, parent->node, exploration );
            else
                stream << "N.A.";
            stream << "<BR/>"
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
            rule.print_move( s, child.node.move );
            const std::string new_name = name + " " + s.str();

            // plot edge to child
            stream << "\"" << name << "\"" << " -- \"" << new_name << "\"\n";

            // apply move
            rule.apply_move( child.node.move, player );

            rec( new_name, child, &node, Player( -player ), depth);
            // undo move
            rule.undo_move( child.node.move, player );
        }
    }

    std::ostream& stream;
    GenericRule< MoveT >& rule;
    double exploration;
    Node< MoveT > const* emph;
    OutStream out_stream;
    DisplayNode display_node;
};

template< typename MoveT >
void lens( GenericRule< MoveT >& initial_rule, montecarlo::Node< MoveT > const& mnode,
           double exploration, Player player )
{
    Node< MoveT > root( mnode );

    size_t depth = 2;
    DisplayNode display_node = Stats;
    TreeType tree_type = Hierarchie;
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
                gv, *path.back(), exploration, *rule, player,
                tree_type, display_node, node, depth );
        }

        char c = 0;
        std::cout << "your choice (q, t, c, +, -, u, d, l, r)?";
        std::cin >> c;
        if (c == 'q')
            break;
        else if (c == 't')
            display_node = DisplayNode( -display_node );
        else if (c == 'c')
            tree_type = TreeType( -tree_type );
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
                rule->undo_move( node->node.move, player );
                player = Player( -player );
            }
        }
        else if (c == 'd') // down
        {
            if (!node->children.empty())
            {
                path.push_back( node );
                rule->apply_move( node->node.move, player );
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
*/

} // namespace montecarlo
