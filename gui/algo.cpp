#include "algo.h"
#include "texture.h"

#include <stdexcept>

using namespace std;
using namespace placeholders;

namespace gui {

Algo::Algo( ::Player player ) : player( player ) {}
Algo::~Algo() {}

void Algo::reset()
{
    // release resources
    tree_texture.reset();
    graphviz_tree.reset();
}    

bool Algo::show_tree_controls(DropDownMenu& dropdown_menu)
{
    Panel panel( "tree controls" );

    const int prev_tree_depth = tree_depth.value;
    const int prev_best_count = best_count.value;
    const int prev_best_percentage = best_percentage.value;

    show_spinner( tree_depth );
    dropdown_menu.add( display_menu );
    dropdown_menu.add( show_nodes );
    if (show_nodes.selected == BestCountIdx)
        show_spinner( best_count );
    else if (show_nodes.selected == BestPercentageIdx)
        show_spinner( best_percentage );
    return 
        display_menu.has_changed() || 
        show_nodes.has_changed() || 
        (prev_tree_depth != tree_depth.value) ||
        (prev_best_count != best_count.value) ||
        (prev_best_percentage != best_percentage.value);
}

bool Algo::has_texture() const 
{ return tree_texture.operator bool(); }

void Algo::draw_texture( 
    float board_width, float board_height, float shift_x, float shift_y, float zoom )
{
    if (tree_texture)
        tree_texture->draw( board_width, board_height, shift_x, shift_y, zoom );
}

void Algo::reset_texture()
{
    if (!graphviz_tree)
        return;
    unique_ptr< ChooseNodes > choose_nodes;
    if (show_nodes.selected == AllIdx)
        choose_nodes.reset( new ChooseAllNodes());
    else if (show_nodes.selected == BestCountIdx)
        choose_nodes.reset( get_choose_best_count_nodes());
    else if (show_nodes.selected == BestPercentageIdx)
        choose_nodes.reset( get_choose_best_percentage_nodes());
    else
        throw runtime_error( "invalid show nodes menu selection");
    tree_texture.reset( 
        new RaylibTexture( graphviz_tree->render_sub_graph(
              display_modes[display_menu.selected], Circular, tree_depth.value, *choose_nodes )));
}

void Algo::refocus_tree( 
    float board_width, float board_height, float shift_x, float shift_y, 
    float zoom, double x, double y )
{
    auto coord = tree_texture->calc_coord( board_width, board_width, shift_x, shift_y, zoom,x, y);

    Agnode_t* gv_node = graphviz_tree->get_node_by_coord( coord.first, coord.second );
    if (gv_node)
    {
        graphviz_tree->set_focus_node( gv_node );
        reset_texture();
    }
}

function< double (GenericRule< tic_tac_toe::Move >&, ::Player) > TicTacToeEval::get_eval_function()
{
    if (eval_menu.selected == 0)
        return [](auto& rule, auto) 
        { return tic_tac_toe::simple_estimate::eval( 
            dynamic_cast< tic_tac_toe::Rule const& >( rule )); };
    else if (eval_menu.selected == 1)
        return [](auto& rule, auto) 
        { return tic_tac_toe::trivial_estimate::eval( 
            dynamic_cast< tic_tac_toe::Rule const& >( rule )); };
    else
        throw runtime_error( "invalid ttt eval menu selection");
}

void TicTacToeEval::show_side_panel(DropDownMenu& dropdown_menu)
{
    dropdown_menu.add( eval_menu );
}

function< double (GenericRule< meta_tic_tac_toe::Move >&, ::Player) > 
    MetaTicTacToeEval::get_eval_function()
{
    if (eval_menu.selected == 0)
        return [this](GenericRule< meta_tic_tac_toe::Move >& rule, ::Player) 
        { return meta_tic_tac_toe::simple_estimate::eval( 
            dynamic_cast< meta_tic_tac_toe::Rule& >( rule ), score_weight.value ); };
    else
        throw runtime_error( "invalid uttt eval menu selection");
}

void MetaTicTacToeEval::show_side_panel(DropDownMenu& dropdown_menu)
{
    dropdown_menu.add( eval_menu );
    show_float_value_box( score_weight );
}

TicTacToeNegamax::TicTacToeNegamax( ::Player player ) : Negamax< tic_tac_toe::Move >( player ) {}

function< double (GenericRule< tic_tac_toe::Move >&, ::Player) > TicTacToeNegamax::get_eval_function()
{ return TicTacToeEval::get_eval_function(); }

void TicTacToeNegamax::show_side_panel(DropDownMenu& dropdown_menu)    
{
    TicTacToeEval::show_side_panel( dropdown_menu);
}

MetaTicTacToeNegamax::MetaTicTacToeNegamax( ::Player player ) 
    : Negamax< meta_tic_tac_toe::Move >( player ) {}

void MetaTicTacToeNegamax::show_side_panel(DropDownMenu& dropdown_menu)
{
    Negamax::show_side_panel( dropdown_menu);
    MetaTicTacToeEval::show_side_panel( dropdown_menu );    
}

function< double (GenericRule< meta_tic_tac_toe::Move >&, ::Player) > 
    MetaTicTacToeNegamax::get_eval_function()
{ return MetaTicTacToeEval::get_eval_function(); }

TicTacToeMinimax::TicTacToeMinimax( ::Player player ) : Minimax< tic_tac_toe::Move >( player ) {}
function< double (GenericRule< tic_tac_toe::Move >&, ::Player) > 
    TicTacToeMinimax::get_eval_function()
{
    return TicTacToeEval::get_eval_function();
}

void TicTacToeMinimax::build_tree( GVC_t* gv_gvc )
{
    auto m_algo = dynamic_cast< MinimaxAlgorithm< tic_tac_toe::Move >* >( algorithm.get());
    if (!m_algo)
        throw std::runtime_error( "invalid algo (build_tree)");

    graphviz_tree.reset( new minimax::TicTacToeTree(
        gv_gvc, player, m_algo->get_root()));
    reset_texture();
}

void TicTacToeMinimax::show_side_panel(DropDownMenu& dropdown_menu)
{
    Minimax< tic_tac_toe::Move >::show_side_panel( dropdown_menu);
    TicTacToeEval::show_side_panel( dropdown_menu );
}

Recursion< tic_tac_toe::Move >* TicTacToeMinimax::get_recursion_function()
{
    if (recursion_menu.selected == 0)
        return new MaxDepth< tic_tac_toe::Move >( depth.value);
    else if (recursion_menu.selected == 1)
        return new MaxVertices< tic_tac_toe::Move > ( max_vertices.value );
    else
        throw runtime_error( "invalid ttt recursion menu selection");
}

function< tic_tac_toe::Move const& (VertexList< tic_tac_toe::Move > const&) > TicTacToeMinimax::get_choose_move_function()
{
    if (choose_menu.selected == 0)
        return bind( &ChooseFirst< tic_tac_toe::Move >::operator(), ChooseFirst< tic_tac_toe::Move >(), _1 );
    else if (choose_menu.selected == 1)
        return [cm = make_shared< ChooseMove< tic_tac_toe::Move > >( bucket_width.value)]
            (VertexList< tic_tac_toe::Move > const& vertices)
            { return (*cm)( vertices ); };
    else
        throw runtime_error( "invalid ttt choose move menu selection");
}

MetaTicTacToeMinimax::MetaTicTacToeMinimax( ::Player player ) 
    : Minimax< meta_tic_tac_toe::Move >( player) {}

function< double (GenericRule< meta_tic_tac_toe::Move >&, ::Player) > 
    MetaTicTacToeMinimax::get_eval_function()
{
    return MetaTicTacToeEval::get_eval_function();
}

void MetaTicTacToeMinimax::build_tree( GVC_t* gv_gvc )
{
    auto m_algo = dynamic_cast< MinimaxAlgorithm< meta_tic_tac_toe::Move >* >( algorithm.get());
    if (!m_algo)
        throw std::runtime_error( "invalid algo (build_tree)");

    graphviz_tree.reset( new minimax::MetaTicTacToeTree(
        gv_gvc, player, m_algo->get_root()));
    reset_texture();
}

void MetaTicTacToeMinimax::show_side_panel(DropDownMenu& dropdown_menu)
{
    Minimax< meta_tic_tac_toe::Move >::show_side_panel( dropdown_menu);
    MetaTicTacToeEval::show_side_panel( dropdown_menu );
}

Recursion< meta_tic_tac_toe::Move >* MetaTicTacToeMinimax::get_recursion_function()
{
    if (recursion_menu.selected == 0)
        return new MaxDepth<meta_tic_tac_toe::Move >( depth.value);
    else if (recursion_menu.selected == 1)
        return new MaxVertices< meta_tic_tac_toe::Move >( max_vertices.value );
    else
        throw runtime_error( "invalid uttt recursion menu selection");
}

function< meta_tic_tac_toe::Move const& (VertexList< meta_tic_tac_toe::Move > const&) > 
    MetaTicTacToeMinimax::get_choose_move_function()
{
    if (choose_menu.selected == 0)
        return bind( &ChooseFirst< meta_tic_tac_toe::Move >::operator(), ChooseFirst< meta_tic_tac_toe::Move >(), _1 );
    else if (choose_menu.selected == 1)
        return [cm = make_shared< ChooseMove< meta_tic_tac_toe::Move > >( bucket_width.value)]
            (VertexList< meta_tic_tac_toe::Move > const& vertices)
            { return (*cm)( vertices ); };
    else
        throw runtime_error( "invalid uttt choose move menu selection");
}

TicTacToeMontecarlo::TicTacToeMontecarlo( ::Player player ) 
    : Montecarlo< tic_tac_toe::Move >( player, Menu { "choose", {"best"}}) {}

void TicTacToeMontecarlo::build_tree( GVC_t* gv_gvc )
{
    auto m_algo = dynamic_cast< montecarlo::Algorithm< tic_tac_toe::Move >* >( algorithm.get());
    if (!m_algo)
        throw std::runtime_error( "invalid algo (build_tree)");

    graphviz_tree.reset( new montecarlo::TicTacToeTree(
        gv_gvc, player, m_algo->get_mcts().exploration, m_algo->get_root()));
    reset_texture();
}

montecarlo::ChooseMove< tic_tac_toe::Move >* TicTacToeMontecarlo::get_choose_move_function()
{
    if (choose_menu.selected == 0)
        return new montecarlo::ChooseBest< tic_tac_toe::Move >();
    else
        throw runtime_error( "invalid ttt montecarlo choose move menu selection");
}

MetaTicTacToeMontecarlo::MetaTicTacToeMontecarlo( ::Player player ) 
    : Montecarlo< meta_tic_tac_toe::Move >( player, Menu { "choose", {"best"}}) {}

void MetaTicTacToeMontecarlo::build_tree( GVC_t* gv_gvc )
{
    auto m_algo = dynamic_cast< montecarlo::Algorithm< meta_tic_tac_toe::Move >* >( algorithm.get());
    if (!m_algo)
        throw std::runtime_error( "invalid algo (build_tree)");

    graphviz_tree.reset( new montecarlo::MetaTicTacToeTree(
        gv_gvc, player, m_algo->get_mcts().exploration, m_algo->get_root()));
    reset_texture();
}

montecarlo::ChooseMove< meta_tic_tac_toe::Move >* 
    MetaTicTacToeMontecarlo::get_choose_move_function()
{
    if (choose_menu.selected == 0)
        return new montecarlo::ChooseBest< meta_tic_tac_toe::Move >();
    else
        throw runtime_error( "invalid uttt montecarlo choose move menu selection");
}

} // namespace gui {
