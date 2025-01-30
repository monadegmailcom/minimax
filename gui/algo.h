#pragma once

#include "../game.h"
#include "../tree.h"

#include "helper.h"

namespace gui {

struct DropDownMenu;
class RaylibTexture;

class Algo
{
public:
    Algo( ::Player );
    virtual ~Algo();
    virtual void build_tree( GVC_t* gv_gvc ) = 0;
    virtual void display_board() = 0;
    virtual Algorithm* get_algorithm() = 0;
    virtual void show_side_panel(DropDownMenu& dropdown_menu) = 0;

    bool has_texture() const;
    void draw_texture( 
        float board_width, float board_height, float shift_x, float shift_y, float zoom );
    void reset_texture();
    // return true, if a node is double clicked
    bool refocus_tree( 
        float board_width, float board_height, float shift_x, float shift_y, 
        float zoom, double x, double y );
    // return true if the tree controls are changed
    virtual bool show_tree_controls(DropDownMenu&);
    virtual ChooseNodes* get_choose_best_count_nodes() = 0;
    virtual ChooseNodes* get_choose_best_percentage_nodes() = 0;
protected:
    void reset();

    std::unique_ptr< RaylibTexture > tree_texture;    
    std::unique_ptr< GraphvizTree > graphviz_tree;

    ::Player player;
    Spinner tree_depth = Spinner( "tree depth", 2, 1, 10 );
    enum DisplayIdx { PNGIdx, MoveIdx, StatsIdx};
    DisplayNode display_modes[3] = { DisplayBoard, DisplayMove, DisplayStats };
    Menu display_menu = Menu { "display", {"board", "move", "stats"}, DisplayStats };
    enum ShowNodesIdx { AllIdx, BestCountIdx, BestPercentageIdx };
    Menu show_nodes = Menu { "show nodes", {"all", "best count", "best percentage"}, BestCountIdx };
    Spinner best_count = Spinner( "best count", 5, 1, 10 );
    Spinner best_percentage = Spinner( "best percentage", 50, 1, 100 );
};

template< typename MoveT >
class AlgoGenerics : public Algo
{
public:
    AlgoGenerics( ::Player player ) : Algo( player ) {}
    virtual ~AlgoGenerics() {}
    void stop_algo()
    {
        if (algorithm)
        {
            // request algo to stop and wait until it finishes
            algorithm->stop();

            // release algo resource
            algorithm.reset();

            Algo::reset();
        }
    }
    Algorithm* get_algorithm() override
    {
        return algorithm.get();
    }

    virtual void start_game( GenericRule< MoveT >& rule ) = 0;
    virtual MoveT const& get_move( Agnode_t* ) = 0;
    std::unique_ptr< AlgorithmGenerics< MoveT > > algorithm;
protected:
    void display_board() override
    {
        auto gv_focus_node = graphviz_tree->get_focus_node();
        std::unique_ptr< GenericRule< MoveT > > focus_rule { 
            update_rule( gv_focus_node ) };
        display_board_rec( *focus_rule, gv_focus_node );
    }

    GenericRule< MoveT >* update_rule( Agnode_t* gv_node )
    {
        if (!gv_node)
            throw std::runtime_error( "invalid node (update_rule)");
        auto parent = agtail( agfstin( graphviz_tree->get_subgraph(), gv_node ));
        GenericRule< MoveT >* rule = 0;
        if (parent)
        {
            rule = update_rule( parent );
            Data* node_data = (Data*)aggetrec(gv_node, "data", 0);
            rule->apply_move( get_move( gv_node ), node_data->player );
        }
        else
            rule = this->rule->clone();

        return rule;
    }

    virtual void draw_board( GenericRule< MoveT >&, std::optional< MoveT > last_move, float board_width,
        float pos_x, float pos_y ) = 0;
    std::unique_ptr< GenericRule< MoveT > > rule;
private:
    void display_board_rec( GenericRule< MoveT >& current_rule, Agnode_t* gv_node )
    {
        const auto coord = ND_coord( gv_node );
        const float size_x = ND_xsize( gv_node );
        const float size_y = ND_ysize( gv_node );

        float size; // the lesser of width and height
        if (size_x > size_y)
            size = size_y;
        else    
            size = size_x;

        draw_board( current_rule, {}, size, coord.x - size / 2, coord.y + size / 2 );

        Data* node_data = (Data*)aggetrec(gv_node, "data", 0);
        auto gv_subgraph = graphviz_tree->get_subgraph();
        for (auto e = agfstout(gv_subgraph, gv_node); e; e = agnxtout(gv_subgraph, e)) 
        {
            auto n = aghead( e );
            MoveT const& move = get_move( n );
            current_rule.apply_move( move, node_data->player );
            display_board_rec( current_rule, n );
            current_rule.undo_move( move, node_data->player );
        }
    }
};

template< typename MoveT >
class Human : public AlgoGenerics< MoveT >
{
public:
    Human( ::Player player ) : AlgoGenerics< MoveT >( player ) {}
protected:
    void start_game( GenericRule< MoveT >& rule ) override
    {
        this->algorithm.reset( new interactive::Algorithm< MoveT >( rule, this->player ));
    }
    void build_tree( GVC_t* )  override {}
    void show_side_panel(DropDownMenu& ) override {}
    bool show_tree_controls(DropDownMenu&) override { return false; }
    ChooseNodes* get_choose_best_count_nodes() override { return nullptr; }
    ChooseNodes* get_choose_best_percentage_nodes() override { return nullptr; }
    MoveT move;
    MoveT const& get_move( Agnode_t* ) override { return move; }
    void draw_board( GenericRule< MoveT >&, std::optional< MoveT > last_move, float board_width,
        float pos_x, float pos_y ) override {}
};

template< typename MoveT >
class MMAlgo : public AlgoGenerics< MoveT >
{
public:
    MMAlgo( ::Player player ) : AlgoGenerics< MoveT >( player ) {}

    void show() {}
    virtual ~MMAlgo() {}
protected:
    virtual std::function< double (GenericRule< MoveT >&, ::Player) > get_eval_function() = 0;
    Spinner depth = Spinner( "depth", 7, 1, 15 );
};

class TicTacToeEval
{
public:
protected:
    std::function< double (GenericRule< tic_tac_toe::Move >&, ::Player) > get_eval_function();
    void show_side_panel(DropDownMenu& dropdown_menu);
    Menu eval_menu {"score heuristic", {"simple estimate", "trivial estimate"}};
};

class MetaTicTacToeEval
{
public:
protected:
    ValueBoxFloat score_weight = ValueBoxFloat( "score weight", "9.0" );
    Menu eval_menu = Menu {"score heuristic", {"simple estimate" }}; 
    std::function< double (GenericRule< meta_tic_tac_toe::Move >&, ::Player) > get_eval_function();
    void show_side_panel(DropDownMenu& dropdown_menu);
};

template< typename MoveT >
class Negamax : public MMAlgo< MoveT >
{
public:
    Negamax( ::Player player ) : MMAlgo< MoveT >( player ) {}
    void start_game( GenericRule< MoveT >& rule ) override
    {
        negamax_algorithm = new NegamaxAlgorithm< MoveT >(
            rule, this->player, this->depth.value, get_reorder_function(), this->get_eval_function());
        this->algorithm.reset( negamax_algorithm ); 
    }
    void show_side_panel(DropDownMenu& dropdown_menu) override
    {
        show_spinner( this->depth );
        dropdown_menu.add( reorder_menu );
    }
    void build_tree( GVC_t* gv_gvc ) override {}
    ChooseNodes* get_choose_best_count_nodes() override { return nullptr; }
    ChooseNodes* get_choose_best_percentage_nodes() override { return nullptr; }
protected:
    NegamaxAlgorithm< MoveT >* negamax_algorithm = nullptr;
    Menu reorder_menu { "reorder moves", {"shuffle", "reorder by score"}, 1 };
    MoveT move;
    MoveT const& get_move( Agnode_t* ) override { return move; }
    void draw_board( GenericRule< MoveT >&, std::optional< MoveT > last_move, 
        float board_width, float pos_x, float pos_y ) override {}
    ReOrder< MoveT > get_reorder_function()
    {
        if (reorder_menu.selected == 0)
            return [shuffle = std::make_shared< Shuffle< MoveT > >()]
                (GenericRule< MoveT >& rule, auto player, auto begin, auto end) 
                { (*shuffle)( rule, player, begin, end ); };
        else if (reorder_menu.selected == 1)
            return [rbs = std::make_shared< ReorderByScore< MoveT > >( this->get_eval_function())]
                (auto& rule, auto player, auto begin, auto end) 
                { (*rbs)( rule, player, begin, end ); };
        else    
            throw std::runtime_error( "invalid reorder menu selection");
    } 
};

class TicTacToeNegamax : public Negamax< tic_tac_toe::Move >, public TicTacToeEval
{
public:
    TicTacToeNegamax( ::Player );
protected:
    std::function< double (GenericRule< tic_tac_toe::Move >&, ::Player) > 
        get_eval_function() override;
    void show_side_panel(DropDownMenu& dropdown_menu) override;
};

class MetaTicTacToeNegamax : public Negamax< meta_tic_tac_toe::Move >, public MetaTicTacToeEval
{
public:
    MetaTicTacToeNegamax( ::Player player );
protected:
    void show_side_panel(DropDownMenu& dropdown_menu) override;
    std::function< double (GenericRule< meta_tic_tac_toe::Move >&, ::Player) > 
        get_eval_function() override;
};

template< typename MoveT >
class Minimax : public MMAlgo< MoveT >
{
public:
    Minimax( ::Player player ) 
    : MMAlgo< MoveT >( player ), 
      recursion_menu( Menu { "recursion", {"max depth", "max vertices"}} ), 
      choose_menu( Menu { "choose", {"best", "epsilon bucket"}} ) {}
    void start_game( GenericRule< MoveT >& rule ) override
    {
        minimax_algorithm = new MinimaxAlgorithm< MoveT >(
            rule, this->player, this->get_eval_function(), get_recursion_function(), get_choose_move_function());
        this->algorithm.reset( minimax_algorithm );
        this->rule.reset( rule.clone());
    }

    ChooseNodes* get_choose_best_count_nodes() override
    {
        auto tree = dynamic_cast< minimax::Tree* >( this->graphviz_tree.get());  
        if (!tree)
            throw std::runtime_error( "invalid tree (Minimax::get_choose_best_count_nodes)");
        return new ChooseBestCountNodes( 
            [tree](Agnode_t* node) {return minimax::get_weight( *tree, node );}, 
            this->best_count.value );
    }

    ChooseNodes* get_choose_best_percentage_nodes() override
    { 
        auto tree = dynamic_cast< minimax::Tree* >( this->graphviz_tree.get());  
        if (!tree)
            throw std::runtime_error( "invalid tree (get_choose_best_percentage_nodes)");
        return new ChooseBestPercentageNodes( 
            [tree](Agnode_t* node) {return minimax::get_weight( *tree, node );}, 
            this->best_percentage.value );
    }
protected:
    Spinner max_vertices = Spinner( "max vertices", 280000, 1, 1000000 );
    enum RecursionIdx { MaxDepthIdx, MaxVerticesIdx };
    Menu recursion_menu;
    enum ChooseIdx { BestIdx, EpsilonBucketIdx };
    Menu choose_menu;
    ValueBoxFloat bucket_width = ValueBoxFloat( "bucket width", "1.00" );
    MinimaxAlgorithm< MoveT >* minimax_algorithm = nullptr;

    MoveT const& get_move( Agnode_t* gv_node ) override 
    {
        Data* node_data = (Data*)aggetrec(gv_node, "data", 0);
        Vertex< MoveT > const& node = *(Vertex< MoveT >*) node_data->node;
 
        return node.move; 
    }
   
    void show_side_panel(DropDownMenu& dropdown_menu) override    
    {    
        dropdown_menu.add( recursion_menu );
        dropdown_menu.add( choose_menu );
        if (recursion_menu.selected == MaxVerticesIdx)
            show_spinner( max_vertices );
        else if (recursion_menu.selected == MaxDepthIdx)
            show_spinner( this->depth );
        if (choose_menu.selected == EpsilonBucketIdx)
            show_float_value_box( bucket_width );
    }
    virtual Recursion< MoveT >* get_recursion_function() = 0;
    virtual std::function< MoveT const& (VertexList< MoveT > const&) > get_choose_move_function() = 0;
};

class TicTacToeMinimax : public Minimax< tic_tac_toe::Move >, public TicTacToeEval
{
public:
    TicTacToeMinimax( ::Player player );
protected:
    std::function< double (GenericRule< tic_tac_toe::Move >&, ::Player) > 
        get_eval_function() override;
    void build_tree( GVC_t* gv_gvc ) override;
    void show_side_panel(DropDownMenu& dropdown_menu) override;

    Recursion< tic_tac_toe::Move >* get_recursion_function() override;
    std::function< tic_tac_toe::Move const& (VertexList< tic_tac_toe::Move > const&) > 
        get_choose_move_function() override;
    void draw_board( GenericRule< tic_tac_toe::Move >&, std::optional< tic_tac_toe::Move > last_move, 
        float board_width, float pos_x, float pos_y ) override;
};

class MetaTicTacToeMinimax : public Minimax< meta_tic_tac_toe::Move >, public MetaTicTacToeEval
{
public:
    MetaTicTacToeMinimax( ::Player );
protected:
    std::function< double (GenericRule< meta_tic_tac_toe::Move >&, ::Player) > 
       get_eval_function() override;
    void build_tree( GVC_t* gv_gvc ) override;
    void show_side_panel(DropDownMenu& dropdown_menu) override;

    Recursion< meta_tic_tac_toe::Move >* get_recursion_function() override;

    std::function< meta_tic_tac_toe::Move const& (VertexList< meta_tic_tac_toe::Move > const&) > 
        get_choose_move_function() override;
    void draw_board( GenericRule< meta_tic_tac_toe::Move >&, std::optional< meta_tic_tac_toe::Move > last_move, 
        float board_width, float pos_x, float pos_y ) override;
};

template< typename MoveT >
class Montecarlo : public AlgoGenerics< MoveT >
{
public:
    Montecarlo( ::Player player, Menu const& choose_menu ) 
    : AlgoGenerics< MoveT >( player ), choose_menu( choose_menu ) {}
    void start_game( GenericRule< MoveT >& rule ) override
    {
        this->algorithm.reset( new montecarlo::Algorithm< MoveT >(
            rule, this->player, this->get_choose_move_function(), simulations.value, 
            exploration_factor.value ));
        this->rule.reset( rule.clone());
    }
protected:
    MoveT const& get_move( Agnode_t* gv_node ) override 
    {
        Data* node_data = (Data*)aggetrec(gv_node, "data", 0);
        montecarlo::Node< MoveT > const& node = *(montecarlo::Node< MoveT >*) node_data->node;
 
        return node.move; 
    }

    virtual montecarlo::ChooseMove< MoveT >* get_choose_move_function() = 0;

    ChooseNodes* get_choose_best_count_nodes() override
    {
        auto tree = dynamic_cast< montecarlo::Tree* >( this->graphviz_tree.get());  
        if (!tree)
            throw std::runtime_error( "invalid tree (get_choose_best_count_nodes)");
        return new ChooseBestCountNodes( 
            [tree](Agnode_t* node) {return montecarlo::get_weight( *tree, node );}, 
            this->best_count.value );
    }
    ChooseNodes* get_choose_best_percentage_nodes() override
    {
        auto tree = dynamic_cast< montecarlo::Tree* >( this->graphviz_tree.get());  
        if (!tree)
            throw std::runtime_error( "invalid tree (get_choose_best_percentage_nodes)");
        return new ChooseBestPercentageNodes( 
            [tree](Agnode_t* node) {return montecarlo::get_weight( *tree, node );}, 
            this->best_percentage.value );
    }

    void show_side_panel(DropDownMenu& dropdown_menu)  override
    {
        dropdown_menu.add( choose_menu );
        show_spinner( simulations );
        show_float_value_box( exploration_factor );
    }
    montecarlo::Algorithm< MoveT >* montecarlo_algorithm = nullptr;
    Menu choose_menu;
    Spinner simulations = Spinner( "simulations", 80000, 1, 1000000 );
    ValueBoxFloat exploration_factor = ValueBoxFloat( "exploration factor", "0.40" );
};

class TicTacToeMontecarlo : public Montecarlo< tic_tac_toe::Move >
{
public: 
    TicTacToeMontecarlo( ::Player );
protected:
    void build_tree( GVC_t* gv_gvc ) override;
    montecarlo::ChooseMove< tic_tac_toe::Move >* get_choose_move_function() override;
    void draw_board( GenericRule< tic_tac_toe::Move >&, std::optional< tic_tac_toe::Move > last_move, 
        float board_width, float pos_x, float pos_y ) override;
};

class MetaTicTacToeMontecarlo : public Montecarlo< meta_tic_tac_toe::Move >
{
public: 
    MetaTicTacToeMontecarlo( ::Player );
    void build_tree( GVC_t* gv_gvc ) override;
protected:
    montecarlo::ChooseMove< meta_tic_tac_toe::Move >* get_choose_move_function() override;
    void draw_board( GenericRule< meta_tic_tac_toe::Move >&, std::optional< meta_tic_tac_toe::Move > last_move, 
        float board_width, float pos_x, float pos_y ) override;
};

} // namespace gui {
