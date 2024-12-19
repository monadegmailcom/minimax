#include "algo.h"

namespace gui {

struct Player
{
    Player( std::string const& name, ::Player player );
    virtual ~Player();

    std::string name;
    ::Player player;
    static const size_t algo_count = 4;
    enum AlgoIdx { HumanIdx, NegamaxIdx, MinimaxIdx, MontecarloIdx };
    Menu algo_menu = Menu { "algorithm", {"human", "negamax", "minimax", "montecarlo"}, MontecarloIdx };

    virtual void show_game_info( bool ticking ) = 0;
    virtual Algo& get_algo() = 0;
};

template< typename MoveT >
class PlayerGenerics : public Player
{
public:
    PlayerGenerics( std::string const& name, ::Player player ) 
        : Player( name, player ) {}
    virtual ~PlayerGenerics() {}
    AlgoGenerics< MoveT >& get_algo_generic()
    {
        return *algos[algo_menu.selected];
    }

    Algo& get_algo()
    {
        return get_algo_generic();
    }

    void show_game_info( bool ticking )
    {
        Panel panel( name.c_str());
        auto& algorithm = *(algos[algo_menu.selected]->algorithm);
        std::chrono::microseconds duration = algorithm.get_duration();
        
        if (ticking)
            duration += std::chrono::duration_cast< std::chrono::microseconds >( 
                std::chrono::steady_clock::now() - algorithm.get_start_time());

        show_label( "algorithm", algo_menu.items[algo_menu.selected]);
        const ldiv_t min = ldiv( duration.count(), 60 * 1000000);
        const ldiv_t sec = ldiv( min.rem, 1000000);
        const long dsec = sec.rem / 100000;
        static std::ostringstream stream; // reuse memory
        stream.str( "" );
        stream << std::setfill( '0' ) << std::setw( 2 ) << min.quot << ":" 
               << std::setw( 2 ) << sec.quot << "." << std::setw( 1 ) << dsec;  
        show_label( "accumulated time", stream.str().c_str());
    }
protected:
    std::unique_ptr< AlgoGenerics< MoveT > > algos[algo_count];
};

class TicTacToePlayer : public PlayerGenerics< tic_tac_toe::Move >
{
public:
    TicTacToePlayer( std::string const& name, ::Player );
};

class MetaTicTacToePlayer : public PlayerGenerics< meta_tic_tac_toe::Move >
{
public:
    MetaTicTacToePlayer( std::string const& name, ::Player player );
};


} // namespace gui {
