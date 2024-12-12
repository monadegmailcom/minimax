#pragma once

#include "negamax.h"
#include "minimax.h"
#include "montecarlo.h"
#include "meta_tic_tac_toe.h"

#include <graphviz/gvc.h>

#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <limits>
#include <iomanip>
#include <cassert>

enum DisplayNode { DisplayBoard, DisplayMove, DisplayStats };
enum Layout { Hierarchie, Circular };

struct RenderData
{
    char* data;
    unsigned length;
    double width;
    double height;
};

class GraphvizTree
{
public:
    GraphvizTree( GVC_t* gv_gvc, Player player );

    virtual ~GraphvizTree();

    RenderData render_sub_graph( 
        DisplayNode _display_node, Layout layout, size_t depth );

    void set_focus_node( Agnode_t* node ) { gv_focus_node = node; }
    Agnode_t* get_node_by_coord( double x, double y );
protected:
    virtual void set_node_attribute( Agnode_t*, Player ) = 0;

    std::ostringstream value; // reuse allocated memory
    DisplayNode display_node = DisplayMove;
    Agraph_t* gv_graph = nullptr;
    Agnode_t* gv_focus_node = nullptr;
    Agraph_t* gv_subgraph = nullptr;
private:
    void add_node_to_subgraph( Agnode_t* gv_node, size_t depth );

    GVC_t* gv_gvc = nullptr;
    Player player;
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
        void* node;
    };

    struct Stats
    {
        double points;
        size_t playouts;
        double cbt;
        size_t depth;
        std::string move;
    };
protected:
    float exploration;
private:
    void set_node_attribute( Agnode_t*, Player );
    virtual void get_stats( Agnode_t* gv_node, Stats& move ) = 0;
};

class TicTacToeTree : public Tree
{
public:
    TicTacToeTree( 
        GVC_t* gv_gvc, Player player, float exploration, montecarlo::Node< tic_tac_toe::Move > const& node );
private:
    virtual void get_stats( Agnode_t* gv_node, Stats& move );
};

class MetaTicTacToeTree : public Tree
{
public:
    MetaTicTacToeTree( 
        GVC_t* gv_gvc, Player player, float exploration, montecarlo::Node< meta_tic_tac_toe::Move > const& node );
private:
    virtual void get_stats( Agnode_t* gv_node, Stats& move );
};

} // namespace montecarlo
