#include "player.h"

using namespace std;

namespace gui {

Player::Player( string const& name, ::Player player ) 
    : name( name ), player( player ) {}
Player::~Player() 
{}

TicTacToePlayer::TicTacToePlayer( string const& name, ::Player player ) 
    : PlayerGenerics( name, player )
{
    algos[Player::HumanIdx] = make_unique< Human< tic_tac_toe::Move > >( player );
    algos[Player::NegamaxIdx] = make_unique< TicTacToeNegamax >( player );
    algos[Player::MinimaxIdx] = make_unique< TicTacToeMinimax >( player );
    algos[Player::MontecarloIdx] = make_unique< TicTacToeMontecarlo >( player );
}

MetaTicTacToePlayer::MetaTicTacToePlayer( string const& name, ::Player player ) 
    : PlayerGenerics( name, player )
{
    algos[Player::HumanIdx] = make_unique< Human< meta_tic_tac_toe::Move > >( player );
    algos[Player::NegamaxIdx] = make_unique< MetaTicTacToeNegamax >( player );
    algos[Player::MinimaxIdx] = make_unique< MetaTicTacToeMinimax >( player );
    algos[Player::MontecarloIdx] = make_unique< MetaTicTacToeMontecarlo >( player );
}

} // namespace gui {

