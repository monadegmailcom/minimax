#include "tree.h"

#include <graphviz/cgraph.h>
#include <graphviz/gvcext.h>

using namespace std;

GraphvizTree::GraphvizTree( GVC_t* gv_gvc, Player player ) : gv_gvc( gv_gvc ), player( player )
{
    gv_graph = agopen((char*)"g", Agstrictdirected, nullptr);
    if (!gv_graph)
        throw std::runtime_error( "could not create graph");
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

Agnode_t* GraphvizTree::get_node_by_coord( double x, double y )
{
    for (auto n = agfstnode(gv_subgraph); n; n = agnxtnode( gv_subgraph, n ))
    {
        auto coord = ND_coord(n);
        double left = coord.x - ND_xsize( n ) / 2;
        double right = coord.x + ND_xsize( n ) / 2;
        double top = coord.y - ND_ysize( n ) / 2;
        double bottom = coord.y + ND_ysize( n ) / 2;

        if (left <= x && x <= right && top <= y && y <= bottom)
            return n; 
    }
    return nullptr;
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

GraphvizTree::RenderData GraphvizTree::render_sub_graph( 
    DisplayNode _display_node, Layout layout, size_t depth )
{
    if (!depth)
        throw std::runtime_error( "invalid depth (render_sub_subgraph)");
    if (gv_subgraph)
    {
        gvFreeLayout(gv_gvc, gv_subgraph);
        agclose(gv_subgraph);
    }
    gv_subgraph = agsubg(gv_graph, (char*)"subgraph", true);
    if (!gv_subgraph)
        throw std::runtime_error( "could not create subgraph (render_subgraph)");
    
    auto gv_edge = agfstin( gv_graph, gv_focus_node );
    if (gv_edge)
    {
        agsubnode(gv_subgraph, agtail( gv_edge ), true);
        agsubedge(gv_subgraph, gv_edge, true);
    }
    
    add_node_to_subgraph( gv_focus_node, depth );

    display_node = _display_node;
    if (!gv_subgraph)
        throw std::runtime_error( "no subgraph (render)");

    agsafeset( gv_focus_node, (char*)"color", (char*)"red", "");

    set_node_attribute( agfstnode(gv_subgraph), player );

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
    
    gvRenderData(gv_gvc, gv_subgraph, "png", &rendered_data, &length);

    agsafeset( gv_focus_node, (char*)"color", "", "");

    boxf box = GD_bb(gv_subgraph);

    return RenderData { 
        .data = rendered_data, 
        .length = length,
        .width = box.UR.x - box.LL.x,
        .height = box.UR.y - box.LL.y};
}

namespace montecarlo
{

Tree::Tree( GVC_t* gv_gvc, Player player, float exploration ) :
    GraphvizTree( gv_gvc, player ), exploration( exploration )
{}

void Tree::set_node_attribute( Agnode_t* gv_node, Player player )
{
    Data* node_data = (Data*)aggetrec(gv_node, "data", 0);

    value.str("");
    if (display_node == SingleMove)
        value << node_data->move;
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
            << "<TABLE BORDER=\"0\" CELLBORDER=\"0\" CELLSPACING=\"0\" CELLPADDING=\"4\">"
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

    agsafeset( gv_node, (char*)"label", agstrdup_html( gv_subgraph, value.str().data()), "");

    if (player == player1)
        agsafeset(gv_node, (char*)"shape", (char*)"box", "");
    else
        agsafeset(gv_node, (char*)"shape", (char*)"ellipse", "");

    for (auto e = agfstout(gv_subgraph, gv_node); e; e = agnxtout(gv_subgraph, e)) 
        set_node_attribute( aghead( e ), Player( -player ));
}

} // namespace montecarlo