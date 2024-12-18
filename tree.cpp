#include "tree.h"

#include <graphviz/cgraph.h>
#include <graphviz/gvcext.h>

using namespace std;

void ChooseAllNodes::operator()( Agraph_t* graph, Agraph_t* sub_graph, Agnode_t* node )
{
    for (auto e = agfstout(graph, node); e; e = agnxtout(graph, e)) 
    {   
        agsubnode(sub_graph, aghead( e ), true);
        agsubedge(sub_graph, e, true);
    }
}

GraphvizTree::GraphvizTree( GVC_t* gv_gvc, Player player ) : gv_gvc( gv_gvc ), player( player )
{
    gv_graph = agopen((char*)"graph", Agstrictdirected, nullptr);
    gv_subgraph = agsubg(gv_graph, (char*)"subgraph", true);

    if (!gv_graph || !gv_subgraph)
        throw std::runtime_error( "could not create graph");
    agsafeset( gv_graph, (char*)"bgcolor", (char*)"transparent", (char*)"");
}

GraphvizTree::~GraphvizTree()
{
    gvFreeLayout(gv_gvc, gv_subgraph);
    agclose(gv_subgraph);
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

void GraphvizTree::add_node_to_subgraph( 
    Agnode_t* gv_node, size_t depth, ChooseNodes& choose_nodes )
{
    --depth;
    if (!depth)
        return;
    choose_nodes( gv_graph, gv_subgraph, gv_node );
    for (auto e = agfstout(gv_subgraph, gv_node); e; e = agnxtout(gv_subgraph, e)) 
        add_node_to_subgraph( aghead( e ), depth, choose_nodes );
}

RenderData GraphvizTree::render_sub_graph( 
    DisplayNode _display_node, Layout layout, size_t depth, ChooseNodes& choose_nodes )
{
    if (!depth)
        throw std::runtime_error( "invalid depth (render_sub_subgraph)");

    gvFreeLayout(gv_gvc, gv_subgraph);
    agclose(gv_subgraph);
    gv_subgraph = agsubg(gv_graph, (char*)"subgraph", true);
    if (!gv_subgraph)
        throw std::runtime_error( "could not create subgraph (render_subgraph)");
    
    agsubnode(gv_subgraph, gv_focus_node, true);
    auto gv_edge = agfstin( gv_graph, gv_focus_node );
    if (gv_edge)
    {
        agsubnode(gv_subgraph, agtail( gv_edge ), true);
        agsubedge(gv_subgraph, gv_edge, true);
    }
    
    add_node_to_subgraph( gv_focus_node, depth, choose_nodes );

    display_node = _display_node;
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

ChooseBestCountNodes::ChooseBestCountNodes( Tree& tree, size_t best_count )
: best_count( best_count ), tree( tree ) {}

void ChooseBestCountNodes::operator()( Agraph_t* graph, Agraph_t* sub_graph, Agnode_t* node )
{
    // copy all outgoing edges
    edges.clear();
    for (auto e = agfstout(graph, node); e; e = agnxtout(graph, e)) 
        edges.push_back( e );

    // sort edge heads descending by playouts
    sort( edges.begin(), edges.end(), 
        [this]( Agedge_t* lhs, Agedge_t* rhs ) 
        {
            this->tree.get_stats( aghead( lhs ), this->stats );
            const size_t lhs_playouts = this->stats.playouts;
            this->tree.get_stats( aghead( rhs ), this->stats );
            const size_t rhs_playouts = this->stats.playouts;
            return lhs_playouts > rhs_playouts;
        });
    
    // add first best_count edges to subgraph
    size_t count = best_count;
    for (auto e : edges)
    {
        if (!count)
            break;
        --count;
        agsubnode(sub_graph, aghead( e ), true);
        agsubedge(sub_graph, e, true);
    }
}

Tree::Tree( GVC_t* gv_gvc, Player player, float exploration ) :
    GraphvizTree( gv_gvc, player ), exploration( exploration )
{}

void Tree::set_node_attribute( Agnode_t* gv_node, Player player )
{
    static Stats stats; // recycle memory for move string
    get_stats( gv_node, stats );

    static ostringstream value; // recycle memory
    value.str("");

    if (display_node == DisplayMove)
        value << stats.move;
    else if (display_node == DisplayBoard)
    {
        // todo
    }
    else if (display_node == DisplayStats)
    {
        const char* const entry_prefix = "<TR><TD ALIGN=\"LEFT\" WIDTH=\"50\">";
        const char* const entry_postfix = "</TD><TD ALIGN=\"RIGHT\" WIDTH=\"50\">";
        const char* const line_postfix = "</TD></TR>";

        value 
            << "<TABLE BORDER=\"0\" CELLBORDER=\"0\" CELLSPACING=\"0\" CELLPADDING=\"4\">"
            << entry_prefix << "move" << entry_postfix << stats.move << line_postfix
            << entry_prefix << "points" << entry_postfix << stats.points << " (" 
            << std::fixed << std::setprecision( 2 ) << stats.points / stats.playouts << ")" << line_postfix
            << entry_prefix << "playouts" << entry_postfix << stats.playouts << line_postfix
            << entry_prefix << "cbt" << entry_postfix << std::fixed << std::setprecision( 2 ) << stats.cbt << line_postfix
            << entry_prefix << "depth" << entry_postfix << stats.depth << line_postfix
            << "</TABLE>";
    }
    else
        throw std::runtime_error( "invalid display node");

    if (value.str().empty())
        agsafeset( gv_node, (char*)"label", (char*)"", "");    
    else
        agsafeset( gv_node, (char*)"label", agstrdup_html( gv_subgraph, value.str().data()), "");

    if (player == player1)
        agsafeset(gv_node, (char*)"shape", (char*)"box", "");
    else
        agsafeset(gv_node, (char*)"shape", (char*)"ellipse", "");

    for (auto e = agfstout(gv_subgraph, gv_node); e; e = agnxtout(gv_subgraph, e)) 
        set_node_attribute( aghead( e ), Player( -player ));
}

template< typename MoveT >
Agnode_t* add_node( Agraph_t* gv_graph, Player player, montecarlo::Node< MoveT > const& node )
{
    Agnode_t* gv_node = agnode(gv_graph, nullptr, true);

    Tree::Data* data = (Tree::Data*) agbindrec( gv_node, "data", sizeof(Tree::Data), false);
    data->depth = 1;
    data->node = (void*)&node;

    for (montecarlo::Node< MoveT > const& child : node.children)
    {
        Agnode_t* gv_child = add_node( gv_graph, Player( -player ), child );
        agedge( gv_graph, gv_node, gv_child, nullptr, true);

        Tree::Data* child_data = (Tree::Data*)aggetrec(gv_child, "data", 0);
        if (child_data->depth > data->depth)
            data->depth = child_data->depth;
    }
    if (!node.children.empty())
        ++data->depth;

    return gv_node;
}

template< typename MoveT >
montecarlo::Node< MoveT >* get_parent_node( Agraph_t* gv_graph, Agnode_t* gv_node )
{
    auto gv_edge = agfstin( gv_graph, gv_node );
    if (gv_edge)
    {
        Agnode_t* gv_parent = agtail( gv_edge );
        if (gv_parent)
        {
            Tree::Data* parent_data = (Tree::Data*)aggetrec(gv_parent, "data", 0);
            if (parent_data)
                return (montecarlo::Node< MoveT >*)parent_data->node;
        }    
    }
    return nullptr;
}

template< typename MoveT >
void get_stats( Agraph_t* gv_graph, Agnode_t* gv_node, double exploration, Tree::Stats& stats )
{
    Tree::Data* node_data = (Tree::Data*)aggetrec(gv_node, "data", 0);
    montecarlo::Node< MoveT > const& node = *(montecarlo::Node< MoveT >*)node_data->node;
    stats.points = node.numerator;
    stats.playouts = node.denominator;
    montecarlo::Node< MoveT >* parent_node = get_parent_node< MoveT >( gv_graph, gv_node );
    stats.cbt = parent_node ? MCTS< MoveT >::cbt( node, *parent_node, exploration ) : 0.0;
    stats.depth = node_data->depth;
    stats.move = to_string( node.move );
}

TicTacToeTree::TicTacToeTree( 
    GVC_t* gv_gvc, Player player, float exploration, montecarlo::Node< tic_tac_toe::Move > const& node ) 
    : Tree( gv_gvc, player, exploration ) 
{
    add_node( gv_graph, player, node );
    gv_focus_node = agfstnode( gv_graph);
}

void TicTacToeTree::get_stats( Agnode_t* gv_node, Tree::Stats& stats )
{
    montecarlo::get_stats< tic_tac_toe::Move >( gv_graph, gv_node, exploration, stats );
}

MetaTicTacToeTree::MetaTicTacToeTree( 
    GVC_t* gv_gvc, Player player, float exploration, montecarlo::Node< meta_tic_tac_toe::Move > const& node ) 
    : Tree( gv_gvc, player, exploration ) 
{
    add_node( gv_graph, player, node );
    gv_focus_node = agfstnode( gv_graph);
}

void MetaTicTacToeTree::get_stats( Agnode_t* gv_node, Tree::Stats& stats )
{
    montecarlo::get_stats< meta_tic_tac_toe::Move >( gv_graph, gv_node, exploration, stats );
}

} // namespace montecarlo