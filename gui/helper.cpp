// for some reasons this has to be defined once
#define RAYGUI_IMPLEMENTATION

#include "helper.h"

using namespace std;

namespace gui {

float panel_y = 0;
const float text_box_height = 30;
float board_width = 600;
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

} // namespace gui {