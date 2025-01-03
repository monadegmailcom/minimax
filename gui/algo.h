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
    virtual Algorithm* get_algorithm() = 0;
    virtual void show_side_panel(DropDownMenu& dropdown_menu) = 0;

    bool has_texture() const;
    void draw_texture( 
        float board_width, float board_height, float shift_x, float shift_y, float zoom );
    void reset_texture();
    void refocus_tree( 
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
    Algorithm* get_algorithm()
    {
        return algorithm.get();
    }

    virtual void start_game( GenericRule< MoveT >& rule ) = 0;

    std::unique_ptr< AlgorithmGenerics< MoveT > > algorithm;
};

template< typename MoveT >
class Human : public AlgoGenerics< MoveT >
{
public:
    Human( ::Player player ) : AlgoGenerics< MoveT >( player ) {}
protected:
    void start_game( GenericRule< MoveT >& rule )
    {
        this->algorithm.reset( new interactive::Algorithm< MoveT >( rule, this->player ));
    }
    void build_tree( GVC_t* ) {}
    void show_side_panel(DropDownMenu& ) {}
    bool show_tree_controls(DropDownMenu&) { return false; }
    ChooseNodes* get_choose_best_count_nodes() { return nullptr; }
    ChooseNodes* get_choose_best_percentage_nodes() { return nullptr; }
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
    void start_game( GenericRule< MoveT >& rule )
    {
        negamax_algorithm = new NegamaxAlgorithm< MoveT >(
            rule, this->player, this->depth.value, get_reorder_function(), get_eval_function());
        this->algorithm.reset( negamax_algorithm ); 
    }
    void show_side_panel(DropDownMenu& dropdown_menu)
    {
        show_spinner( this->depth );
        dropdown_menu.add( reorder_menu );
    }
    void build_tree( GVC_t* gv_gvc ) {}
    ChooseNodes* get_choose_best_count_nodes() { return nullptr; }
    ChooseNodes* get_choose_best_percentage_nodes() { return nullptr; }
protected:
    NegamaxAlgorithm< MoveT >* negamax_algorithm = nullptr;
    Menu reorder_menu { "reorder moves", {"shuffle", "reorder by score"}, 1 };
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

    virtual std::function< double (GenericRule< MoveT >&, ::Player) > get_eval_function() = 0;
};

class TicTacToeNegamax : public Negamax< tic_tac_toe::Move >, public TicTacToeEval
{
public:
    TicTacToeNegamax( ::Player );
protected:
    std::function< double (GenericRule< tic_tac_toe::Move >&, ::Player) > get_eval_function();
    virtual void show_side_panel(DropDownMenu& dropdown_menu);
};

class MetaTicTacToeNegamax : public Negamax< meta_tic_tac_toe::Move >, public MetaTicTacToeEval
{
public:
    MetaTicTacToeNegamax( ::Player player );
protected:
    virtual void show_side_panel(DropDownMenu& dropdown_menu);
    std::function< double (GenericRule< meta_tic_tac_toe::Move >&, ::Player) > get_eval_function();
};

template< typename MoveT >
class Minimax : public MMAlgo< MoveT >
{
public:
    Minimax( ::Player player ) 
    : MMAlgo< MoveT >( player ), 
      recursion_menu( Menu { "recursion", {"max depth", "max vertices"}} ), 
      choose_menu( Menu { "choose", {"best", "epsilon bucket"}} ) {}
    void start_game( GenericRule< MoveT >& rule )
    {
        minimax_algorithm = new MinimaxAlgorithm< MoveT >(
            rule, this->player, this->get_eval_function(), get_recursion_function(), get_choose_move_function());
        this->algorithm.reset( minimax_algorithm );
    }

    ChooseNodes* get_choose_best_count_nodes()
    {
        auto tree = dynamic_cast< minimax::Tree* >( this->graphviz_tree.get());  
        if (!tree)
            throw std::runtime_error( "invalid tree (Minimax::get_choose_best_count_nodes)");
        return new ChooseBestCountNodes( 
            [tree](Agnode_t* node) {return minimax::get_weight( *tree, node );}, 
            this->best_count.value );
    }

    ChooseNodes* get_choose_best_percentage_nodes() 
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
    
    virtual void show_side_panel(DropDownMenu& dropdown_menu)    
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
    std::function< double (GenericRule< tic_tac_toe::Move >&, ::Player) > get_eval_function();
    void build_tree( GVC_t* gv_gvc );
    void show_side_panel(DropDownMenu& dropdown_menu);

    Recursion< tic_tac_toe::Move >* get_recursion_function();
    std::function< tic_tac_toe::Move const& (VertexList< tic_tac_toe::Move > const&) > 
        get_choose_move_function();
};

class MetaTicTacToeMinimax : public Minimax< meta_tic_tac_toe::Move >, public MetaTicTacToeEval
{
public:
    MetaTicTacToeMinimax( ::Player );
protected:
    std::function< double (GenericRule< meta_tic_tac_toe::Move >&, ::Player) > get_eval_function();
    void build_tree( GVC_t* gv_gvc );
    void show_side_panel(DropDownMenu& dropdown_menu);

    Recursion< meta_tic_tac_toe::Move >* get_recursion_function();

    std::function< meta_tic_tac_toe::Move const& (VertexList< meta_tic_tac_toe::Move > const&) > 
        get_choose_move_function();
};

template< typename MoveT >
class Montecarlo : public AlgoGenerics< MoveT >
{
public:
    Montecarlo( ::Player player, Menu const& choose_menu ) 
    : AlgoGenerics< MoveT >( player ), choose_menu( choose_menu ) {}
    void start_game( GenericRule< MoveT >& rule )
    {
        this->algorithm.reset( new montecarlo::Algorithm< MoveT >(
            rule, this->player, this->get_choose_move_function(), simulations.value, 
            exploration_factor.value ));
    }
protected:
    virtual montecarlo::ChooseMove< MoveT >* get_choose_move_function() = 0;

    ChooseNodes* get_choose_best_count_nodes()
    {
        auto tree = dynamic_cast< montecarlo::Tree* >( this->graphviz_tree.get());  
        if (!tree)
            throw std::runtime_error( "invalid tree (get_choose_best_count_nodes)");
        return new ChooseBestCountNodes( 
            [tree](Agnode_t* node) {return montecarlo::get_weight( *tree, node );}, 
            this->best_count.value );
    }
    ChooseNodes* get_choose_best_percentage_nodes()
    {
        auto tree = dynamic_cast< montecarlo::Tree* >( this->graphviz_tree.get());  
        if (!tree)
            throw std::runtime_error( "invalid tree (get_choose_best_percentage_nodes)");
        return new ChooseBestPercentageNodes( 
            [tree](Agnode_t* node) {return montecarlo::get_weight( *tree, node );}, 
            this->best_percentage.value );
    }

    void show_side_panel(DropDownMenu& dropdown_menu)
    {
        dropdown_menu.add( choose_menu );
        show_spinner( simulations );
        show_float_value_box( exploration_factor );
    }
    montecarlo::Algorithm< MoveT >* montecarlo_algorithm = nullptr;
    Menu choose_menu;
    Spinner simulations = Spinner( "simulations", 100 /*80000*/, 1, 1000000 );
    ValueBoxFloat exploration_factor = ValueBoxFloat( "exploration factor", "0.40" );
};

class TicTacToeMontecarlo : public Montecarlo< tic_tac_toe::Move >
{
public: 
    TicTacToeMontecarlo( ::Player );
protected:
    void build_tree( GVC_t* gv_gvc );
    montecarlo::ChooseMove< tic_tac_toe::Move >* get_choose_move_function();
};

class MetaTicTacToeMontecarlo : public Montecarlo< meta_tic_tac_toe::Move >
{
public: 
    MetaTicTacToeMontecarlo( ::Player );
    void build_tree( GVC_t* gv_gvc );
protected:
    montecarlo::ChooseMove< meta_tic_tac_toe::Move >* get_choose_move_function();
};

} // namespace gui {
