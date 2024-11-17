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
    agsafeset(gv_graph, (char*)"bgcolor", (char*)"transparent", (char*)"");
}

GraphvizTree::~GraphvizTree()
{
    gvFreeLayout(gv_gvc, gv_graph);
    agclose(gv_graph);
}

pair< char*, unsigned > GraphvizTree::render( DisplayNode _display_node, Layout layout )
{
    display_node = _display_node;
    set_node_attribute( agfstnode(gv_graph));

    char const* engine;
    if (layout == Hierarchie)
        engine = "dot";
    else if (layout == Circular)
        engine = "circo";
    else
        throw std::runtime_error( "invalid layout");

    gvLayout(gv_gvc, gv_graph, engine );
    
    char* rendered_data;
    unsigned length;        
    gvRenderData(gv_gvc, gv_graph, "svg", &rendered_data, &length);

    for (Agnode_t* node = agfstnode(gv_graph); node; node = agnxtnode(gv_graph, node)) 
    {
        montecarlo::Tree::Data* data = (montecarlo::Tree::Data*)aggetrec(node, "data", 0);
        if (!data)
            throw std::runtime_error( "no data found");
        std::cout << "move: " << data->move << std::endl;
         
        pointf coord = ND_coord(node);
        std::cout << "x: " << coord.x << ", y: " << coord.y << std::endl;
    }

    return { rendered_data, length };
}

namespace montecarlo
{

Tree::Tree( GVC_t* gv_gvc, Player player, float exploration ) :
    GraphvizTree( gv_gvc, player ), exploration( exploration )
{}

void Tree::set_node_attribute( Agnode_t* gv_node )
{
    Data* node_data = (Data*) AGDATA(gv_node);

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
        auto e = agfstin( gv_graph, gv_node );
        auto parent = e ? agtail( e ) : nullptr;
        auto parent_data = parent ? (montecarlo::Tree::Data*)aggetrec(parent, "data", 0) : nullptr;
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

    agsafeset( gv_node, (char*)"label", agstrdup_html( gv_graph, value.str().data()), (char*)"");

    for (auto e = agfstout(gv_graph, gv_node); e; e = agnxtout(gv_graph, e)) 
        set_node_attribute( aghead( e ));
}

/* 
void print_preamble( ostream& stream, TreeType type )
{
    stream << "graph T {\n"
           << "node [fontname=\"Courier New\"]\n";
    if (type == Circular)
        stream << "graph [layout=circo]\n";
}

void print_closing( ostream& stream )
{
    stream << "}\n\n" << flush;
}
 */

} // namespace montecarlo