// for some reasons this has to be defined once
#define RAYGUI_IMPLEMENTATION

#include "helper.h"

using namespace std;

namespace gui {

float panel_y = 0;
const float text_box_height = 30;
float board_width = 800;
const float panel_spacer = 10;
float panel_width = 200;
float panel_x = board_width + panel_spacer;
float window_height = board_width;
float window_width = board_width + panel_width;

Menu::Menu( char const* name, vector< char const* > const& items, int selected ) 
    : name( name ), selected( selected ), prev_selected( selected ), items( items ) 
{
    if (!items.empty())
    {
        str = items.front();
        for (auto itr = items.begin() + 1; itr != items.end(); ++itr)
            str += ";" + string( *itr );
    }
}

void Menu::add_item( string const& item )
{
    items.push_back( item.c_str());
    str += ";" + item;
}

bool Menu::has_changed()
{
    if (selected != prev_selected)
    {
        prev_selected = selected;
        return true;
    }
    return false;
}

ValueBoxFloat::ValueBoxFloat( string const& label, string const& _text ) 
    : label( label ), text( _text.begin(), _text.end())
{ 
    text.push_back( 0 ); 
    value = TextToFloat( _text.c_str());
}

Spinner::Spinner( string const& label, int value, int min, int max ) 
    : label( label ), value( value ), min( min ), max( max ) {}

DropDownMenu::DropDownMenu() 
{ menus.clear(); }

DropDownMenu::~DropDownMenu()
{
    for (auto itr = menus.rbegin(); itr != menus.rend(); ++itr)
    {
        Menu& menu = *itr->first;
        if (distance( itr, menus.rend() ) <= first_disabled_idx)
            GuiSetState(STATE_NORMAL);
    
        const float height = text_box_height + 2 * panel_spacer;
        Rectangle rect = { panel_x, itr->second, panel_width - 2 * panel_spacer, height };
        GuiGroupBox( rect, menu.name );
        rect.x += panel_spacer;
        rect.y += panel_spacer;
        rect.width -= 2 * panel_spacer;
        rect.height -= 2 * panel_spacer;

        if (GuiDropdownBox( rect, menu.str.c_str(), &menu.selected, menu.dropped_down))
            menu.dropped_down = !menu.dropped_down;
        if (distance( itr, menus.rend() ) <= first_disabled_idx)
            GuiSetState(STATE_DISABLED);
    }

    GuiSetState(STATE_NORMAL);
}

void DropDownMenu::add( Menu& menu )
{
    if (menu.dropped_down)
    {
        GuiSetState(STATE_DISABLED);
        first_disabled_idx = min( first_disabled_idx, menus.size()) + 1;
    }

    menus.push_back( make_pair( &menu, panel_y ));
    panel_y += text_box_height + 3 * panel_spacer;
}

vector< pair< Menu*, float > > DropDownMenu::DropDownMenu::menus = {};

bool show_button( const char* text )
{
    const float height = text_box_height;
    Rectangle rect = { panel_x, panel_y, panel_width - 2 * panel_spacer, height };
    panel_y += height + panel_spacer;

    return GuiButton( rect, text );
}

bool show_button( const char* label, string const& text )
{
    const float height = text_box_height + 2 * panel_spacer;
    Rectangle rect = { panel_x, panel_y, panel_width - 2 * panel_spacer, height };
    GuiGroupBox( rect, label );
    rect.x += panel_spacer;
    rect.y += panel_spacer;
    rect.width -= 2 * panel_spacer;
    rect.height -= 2 * panel_spacer;
    panel_y += height + panel_spacer;

    return GuiButton( rect, text.c_str());
}

void show_label( const char* label, const char* text )
{
    Rectangle rect = { panel_x, panel_y, panel_width - 2 * panel_spacer, text_box_height };
    GuiGroupBox( rect, label );
    rect.x += (panel_width - 2 * panel_spacer - GetTextWidth( text )) / 2;
    GuiLabel( rect, text );

    panel_y += text_box_height + panel_spacer;
}

void show_spinner( Spinner& spinner )
{
    const float height = text_box_height + 2 * panel_spacer;
    Rectangle rect = { panel_x, panel_y, panel_width - 2 * panel_spacer, height };
    GuiGroupBox( rect, spinner.label.c_str());
    rect.x += panel_spacer;
    rect.y += panel_spacer;
    rect.width -= 2 * panel_spacer;
    rect.height -= 2 * panel_spacer;
    panel_y += height + panel_spacer;

    if (GuiSpinner( rect, "", &spinner.value, spinner.min, spinner.max, spinner.edit_mode ))
        spinner.edit_mode = !spinner.edit_mode;
}

void show_float_value_box( ValueBoxFloat& value_box )
{
    const float height = text_box_height + 2 * panel_spacer;
    Rectangle rect = { panel_x, panel_y, panel_width - 2 * panel_spacer, height };
    GuiGroupBox( rect, value_box.label.c_str());
    rect.x += panel_spacer;
    rect.y += panel_spacer;
    rect.width -= 2 * panel_spacer;
    rect.height -= 2 * panel_spacer;
    panel_y += height + panel_spacer;

    if (GuiValueBoxFloat( rect, 0, value_box.text.data(), &value_box.value, value_box.edit_mode))
        value_box.edit_mode = !value_box.edit_mode;
}

Panel::Panel( const char* label ) : label( label ) 
{
    panel_y += panel_spacer;

    // adjust x pos and width for the panel
    panel_x += panel_spacer;
    panel_width -= 2 * panel_spacer;
}

Panel::~Panel()
{
    // reset x pos and width
    panel_x -= panel_spacer;
    panel_width += 2 * panel_spacer;

    const float height = panel_y - y_pos;
    Rectangle rect = { panel_x, y_pos, panel_width - 2 * panel_spacer, height };
    GuiGroupBox( rect, label );
    rect.x += panel_spacer;
    rect.y += panel_spacer;
    rect.width -= 2 * panel_spacer;
    rect.height -= 2 * panel_spacer;

    panel_y += panel_spacer;
}

DisableGui::DisableGui( bool disable ) : state( GuiGetState())
{ 
    if (disable)
        GuiSetState(STATE_DISABLED); 
}

DisableGui::~DisableGui() 
{ 
    GuiSetState(state); 
}

void DisableGui::operator()()
{
    GuiSetState(STATE_DISABLED);
}

void draw_box( 
    int i, int j, Color color, float cell_size, float pos_x, float pos_y, float line_width )
{
    pos_x += j * cell_size;
    pos_y += i * cell_size;

    DrawRectangleLinesEx({ pos_x, pos_y, cell_size, cell_size}, line_width, color);
}

void draw_player(
    ::Player player, int i, int j, Color color, float cell_size, float pos_x, float pos_y )
{
    const float spacer = cell_size / 4;
    const float line_width = cell_size / 20;

    pos_x += j * cell_size;
    pos_y += i * cell_size;

    if (player == player1)
    {    
        DrawLineEx(
            {pos_x + spacer, pos_y + spacer}, 
            {pos_x + cell_size - spacer, pos_y + cell_size - spacer}, 
            line_width, color);
        DrawLineEx(
            {pos_x + cell_size - spacer, pos_y + spacer}, 
            {pos_x + spacer, pos_y + cell_size - spacer}, 
            line_width, color);
    }
    else if (player == player2)
    {
        DrawRing( 
            {pos_x + cell_size / 2, pos_y + cell_size / 2}, 
            cell_size / 2 - spacer - line_width, cell_size / 2 - spacer, 0, 360, 36, color);
    }
}

void draw_tic_tac_toe_board( 
    ::Player const* board, optional< tic_tac_toe::Move > const& last_move, float board_width, float pos_x, float pos_y )
{
    const float cell_size = board_width / 3;
    for (int i = 0; i < tic_tac_toe::n; i++)
        for (int j = 0; j < tic_tac_toe::n; j++)
        {
            const int idx = i * tic_tac_toe::n + j;
            const ::Player player = board[idx];
            const Color player_color = last_move == idx ? RED : BLACK;
            draw_box( i, j, BLACK, cell_size, pos_x, pos_y );
            draw_player(player, i, j, player_color, cell_size, pos_x, pos_y);
        }
}

void draw_meta_tic_tac_toe_board( 
    ::Player const* board, ::Player* meta_board, array< bool, meta_tic_tac_toe::item_size >& terminals,
    optional< tic_tac_toe::Move > const& last_move, float board_width, float pos_x, float pos_y )
{
    const float outer_cell_size = board_width / 3;
    const float inner_cell_size = outer_cell_size / 3;
    int idx = 0;
    for (int i = 0; i < meta_tic_tac_toe::n; i++)
        for (int j = 0; j < meta_tic_tac_toe::n; j++)
        {
            const bool terminal = terminals[i * meta_tic_tac_toe::n + j];

            draw_box( i, j, BLACK, outer_cell_size, pos_x, pos_y, 2 );
            const int pos_x2 = pos_x + j * outer_cell_size;
            const int pos_y2 = pos_y + i * outer_cell_size;
            for (int i2 = 0; i2 < meta_tic_tac_toe::n; i2++)
                for (int j2 = 0; j2 < meta_tic_tac_toe::n; j2++)
                {
                    draw_box( i2, j2, BLACK, inner_cell_size, pos_x2, pos_y2, 1.0);
                    const ::Player player = board[idx];
                    const Color LIGHTRED { 255, 127, 127, 255 };
                    const Color player_color = 
                        last_move == idx ? (terminal ? LIGHTRED : RED) : (terminal ? LIGHTGRAY : BLACK);
                    ++idx;
                    draw_player(player, i2, j2, player_color, inner_cell_size, pos_x2, pos_y2);
                }
            
            if (terminal)
                draw_player( meta_board[i * meta_tic_tac_toe::n + j], i, j, BLACK, outer_cell_size, pos_x, pos_y);
        }
}

} // namespace gui {