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

ChooseBestNodes::ChooseBestNodes( function< float( Agnode_t* ) > get_weight )
: get_weight( get_weight ) {}

ChooseBestNodes::~ChooseBestNodes() {}

void ChooseBestNodes::operator()( Agraph_t* graph, Agraph_t* sub_graph, Agnode_t* node )
{
    // copy all outgoing edges
    edges.clear();
    float maxim = 0;
    for (auto e = agfstout(graph, node); e; e = agnxtout(graph, e)) 
    {
        float weight = get_weight( aghead( e ));
        if (weight > 1000)
            weight = 1000;
        if (weight < -1000)
            weight = -1000;
        const float abs = fabs( weight );
        if (abs > maxim)
            maxim = abs;

        edges.push_back( std::make_pair( e, weight ));
    }
    if (maxim == 0.0)
        maxim = 1;

    if (!edges.empty())
        for (auto& e : edges)
            // normalize to [1, 5]
            //e.second = 1.0 + 4 / (1 + exp( -2 * e.second / maxim));
            e.second = 2.5 * (1 + e.second / maxim);

    // sort edge heads descending by weights
    sort( edges.begin(), edges.end(), 
        []( auto& lhs, auto& rhs ) {return lhs.second > rhs.second; });
    
    shrink_edges();

    // add best edges to subgraph
    for (auto& e : edges)
    {
        agsubnode(sub_graph, aghead( e.first ), true);
        agsubedge(sub_graph, e.first, true);
        agsafeset( e.first, (char*)"arrowsize", agstrdup( sub_graph, 
                   to_string( e.second ).data()), "");
    }
}

ChooseBestCountNodes::ChooseBestCountNodes( 
    function< float( Agnode_t* ) > get_weight, size_t best_count )
: ChooseBestNodes( get_weight ), best_count( best_count ) {}

ChooseBestCountNodes::~ChooseBestCountNodes() {}

void ChooseBestCountNodes::shrink_edges()
{
   // add first best_count edges to subgraph
    size_t count = best_count;
    auto itr = edges.begin();
    while (count && itr != edges.end())
    {
        ++itr;
        --count;
    }
    edges.erase( itr, edges.end());
 }

ChooseBestPercentageNodes::ChooseBestPercentageNodes( 
    function< float( Agnode_t* ) > get_weight, size_t best_percentage )
: ChooseBestNodes( get_weight ), best_ratio( (float) best_percentage / 100 ) {}

ChooseBestPercentageNodes::~ChooseBestPercentageNodes() {}

void ChooseBestPercentageNodes::shrink_edges()
{
    float ratio = 0;
    auto itr = edges.begin();
    while (ratio <= best_ratio && itr != edges.end())
    {
        ratio += itr->second;
        ++itr;
    }

    edges.erase( itr, edges.end());
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

namespace minimax
{

float get_weight( Tree& tree, Agnode_t* gv_node )
{
    Tree::Data* node_data = (Tree::Data*)aggetrec(gv_node, "data", 0);
    // the template type char is only a placeholder, it is not used
    Vertex< char > const& node = *(Vertex< char >*)node_data->node;
    return (float) node.value * tree.get_player();
}

template< typename MoveT >
void get_stats( Agraph_t* gv_graph, Agnode_t* gv_node, Tree::Stats& stats )
{
    Tree::Data* node_data = (Tree::Data*)aggetrec(gv_node, "data", 0);
    Vertex< MoveT > const& node = *(Vertex< MoveT >*)node_data->node;
    stats.value = node.value;
    stats.depth = node_data->depth;
    stats.is_terminal = node.is_terminal;
    stats.move = to_string( node.move );
}

template< typename MoveT >
Agnode_t* add_node( Agraph_t* gv_graph, Player player, Vertex< MoveT > const& node )
{
    Agnode_t* gv_node = agnode(gv_graph, nullptr, true);

    Tree::Data* data = (Tree::Data*) agbindrec( gv_node, "data", sizeof(Tree::Data), false);
    data->depth = 1;
    data->node = (void*)&node;

    for (Vertex< MoveT > const& child : node.children)
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

Tree::Tree( GVC_t* gv_gvc, Player player ) : GraphvizTree( gv_gvc, player ) {}

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
            << entry_prefix << "value" << entry_postfix 
            << std::fixed << std::setprecision( 2 ) << stats.value << line_postfix
            << entry_prefix << "depth" << entry_postfix << stats.depth << line_postfix
            << entry_prefix << "is_terminal" << entry_postfix << stats.is_terminal << line_postfix
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

TicTacToeTree::TicTacToeTree( GVC_t* gv_gvc, Player player, Vertex< tic_tac_toe::Move > const& node )
    : Tree( gv_gvc, player ) 
{
    add_node( gv_graph, player, node );
    gv_focus_node = agfstnode( gv_graph);
}

TicTacToeTree::~TicTacToeTree() {}

void TicTacToeTree::get_stats( Agnode_t* gv_node, Tree::Stats& stats )
{
    minimax::get_stats< tic_tac_toe::Move >( gv_graph, gv_node, stats );
}

MetaTicTacToeTree::MetaTicTacToeTree( GVC_t* gv_gvc, Player player, Vertex< meta_tic_tac_toe::Move > const& node )
    : Tree( gv_gvc, player ) 
{
    add_node( gv_graph, player, node );
    gv_focus_node = agfstnode( gv_graph);
}

MetaTicTacToeTree::~MetaTicTacToeTree() {}

void MetaTicTacToeTree::get_stats( Agnode_t* gv_node, Tree::Stats& stats )
{
    minimax::get_stats< tic_tac_toe::Move >( gv_graph, gv_node, stats );
}

} // namespace minimax

namespace montecarlo
{

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

float get_weight( Tree& tree, Agnode_t* gv_node )
{
    Tree::Data* node_data = (Tree::Data*)aggetrec(gv_node, "data", 0);
    // the template type char is only a placeholder, it is not used
    montecarlo::Node< char > const& node = *(montecarlo::Node< char >*)node_data->node;
    montecarlo::Node< char >* parent_node = 
        montecarlo::get_parent_node< char >( tree.get_graph(), gv_node );
    float value = (float) node.denominator;
    if (parent_node)
        value /= parent_node->denominator;
    return value;
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