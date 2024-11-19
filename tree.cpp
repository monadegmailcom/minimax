#include "tree.h"

#include <graphviz/cgraph.h>
#include <graphviz/gvcext.h>

using namespace std;

GraphvizTree::GraphvizTree( GVC_t* gv_gvc, Player player ) : gv_gvc( gv_gvc )
{
    gv_graph = agopen((char*)"g", Agstrictdirected, nullptr);
    if (!gv_graph)
        throw std::runtime_error( "could not create graph");
    char const* player_name = player == player1 ? "player1" : "player2";
    agsafeset( gv_graph, (char*)"label", (char*)player_name, (char*)"");
    agsafeset( gv_graph, (char*)"bgcolor", (char*)"transparent", (char*)"");
}

GraphvizTree::~GraphvizTree()
{
    if (gv_subgraph)
    {
        gvFreeLayout(gv_gvc, gv_subgraph);
        agclose(gv_subgraph);
    }
    agclose(gv_graph);
}

pointf GraphvizTree::get_focus_coord() const
{
    return ND_coord(gv_focus_node);
}

void GraphvizTree::create_subgraph( size_t depth )
{
    if (!depth)
        throw std::runtime_error( "invalid depth (set_subgraph)");
    if (gv_subgraph)
    {
        gvFreeLayout(gv_gvc, gv_subgraph);
        agclose(gv_subgraph);
    }
    gv_subgraph = agsubg(gv_graph, (char*)"subgraph", true);
    if (!gv_subgraph)
        throw std::runtime_error( "could not create subgraph (set_subgraph)");
    add_node_to_subgraph( gv_focus_node, depth );
}

void GraphvizTree::add_node_to_subgraph( Agnode_t* gv_node, size_t depth )
{
    agsubnode(gv_subgraph, gv_node, true);
    --depth;
    if (!depth)
        return;
    for (auto e = agfstout(gv_graph, gv_node); e; e = agnxtout(gv_graph, e)) 
    {
        agsubedge(gv_subgraph, e, true);
        add_node_to_subgraph( aghead( e ), depth );
    }
}

pair< char*, unsigned > GraphvizTree::render( DisplayNode _display_node, Layout layout )
{
    display_node = _display_node;
    if (!gv_subgraph)
        throw std::runtime_error( "no subgraph (render)");
    set_node_attribute( agfstnode(gv_subgraph));

    char const* engine;
    if (layout == Hierarchie)
        engine = "dot";
    else if (layout == Circular)
        engine = "circo";
    else
        throw std::runtime_error( "invalid layout");

    gvLayout(gv_gvc, gv_subgraph, engine );
    
    char* rendered_data;
    unsigned length;        
    gvRenderData(gv_gvc, gv_subgraph, "svg", &rendered_data, &length);

    return { rendered_data, length };
}

namespace montecarlo
{

Tree::Tree( GVC_t* gv_gvc, Player player, float exploration ) :
    GraphvizTree( gv_gvc, player ), exploration( exploration )
{}

void Tree::set_node_attribute( Agnode_t* gv_node )
{
    Data* node_data = (Data*)aggetrec(gv_node, "data", 0);

    value.str("");
    if (display_node == SingleMove)
        ;// TODO: implement
    else if (display_node == Board)
    {
        /* TODO implement
            rule.apply_move( child.move, player );
            rule.undo_move( child.move, player );
        */
    }
    else if (display_node == Stats)
    {
        const char* const entry_prefix = "<TR><TD ALIGN=\"LEFT\" WIDTH=\"50\">";
        const char* const entry_postfix = "</TD><TD ALIGN=\"RIGHT\" WIDTH=\"50\">";
        const char* const line_postfix = "</TD></TR>";
        auto e = agfstin( gv_subgraph, gv_node );
        auto parent = e ? agtail( e ) : nullptr;
        auto parent_data = parent ? (Data*)aggetrec(parent, "data", 0) : nullptr;
        value 
            << "<TABLE BORDER=\"0\" CELLBORDER=\"1\" CELLSPACING=\"0\" CELLPADDING=\"4\">"
            << entry_prefix << "move" << entry_postfix << node_data->move << line_postfix
            << entry_prefix << "points" << entry_postfix << node_data->numerator << " (" 
            << std::fixed << std::setprecision( 2 ) << node_data->numerator / node_data->denominator << ")" << line_postfix
            << entry_prefix << "playouts" << entry_postfix << node_data->denominator << line_postfix
            << entry_prefix << "cbt" << entry_postfix << std::fixed << std::setprecision( 2 ) << node_data->cbt << line_postfix
            << entry_prefix << "depth" << entry_postfix << node_data->depth << line_postfix
            << entry_prefix << "size" << entry_postfix << node_data->size << " ("
            << std::fixed << std::setprecision( 2 ) << (parent_data ? double( node_data->size ) / parent_data->size : 1.0) 
            << ")" << line_postfix
            << "</TABLE>";
    }
    else
        throw std::runtime_error( "invalid display node");

    agsafeset( gv_node, (char*)"label", agstrdup_html( gv_subgraph, value.str().data()), (char*)"");

    for (auto e = agfstout(gv_subgraph, gv_node); e; e = agnxtout(gv_subgraph, e)) 
        set_node_attribute( aghead( e ));
}

} // namespace montecarlo