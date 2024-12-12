#pragma once

#include "raygui.h"

#include <vector>
#include <string>

namespace gui {

extern float panel_y;
extern float board_width;
extern const float panel_spacer;
extern float panel_width;
extern float panel_x;
extern float window_height;
extern float window_width;

struct Menu 
{
    Menu( char const* name, std::vector< char const* > const& items, int selected = 0 );

    void add_item( std::string const& item );
    bool has_changed();

    char const* name;
    bool dropped_down = false;
    int selected;
    int prev_selected;

    std::vector< char const* > items;
    std::string str;
};

struct ValueBoxFloat
{
    ValueBoxFloat( std::string const& label, std::string const& _text );
    std::string label;
    std::vector< char > text;
    float value;
    bool edit_mode = false;
};

struct Spinner
{
    Spinner( std::string const& label, int value, int min, int max ); 

    std::string label;
    int value;
    int min;
    int max;
    bool edit_mode = false;
};

/* a dropdown menu has to be drawn after the elements
   below it to ensure that it is drawn on top, so we
   save the y position of the dropdown menus in a vector
   and draw them in reverse order. Additionally the gui
   state of all gui controls below except a dropped down menu
   have to be STATE_DISABLED. */
struct DropDownMenu
{
    DropDownMenu();
    ~DropDownMenu();

    void add( Menu& menu );

    // make static so we do not have to reallocate memory each time
    // we draw the dropdown menus
    static std::vector< std::pair< Menu*, float > > menus; 
    size_t first_disabled_idx = 1000; // just a large number
};

bool show_button( const char* text );

bool show_button( const char* label, std::string const& text );

void show_label( const char* label, const char* text );

void show_spinner( Spinner& spinner );

void show_float_value_box( ValueBoxFloat& value_box );

struct Panel
{
    Panel( const char* label );
    ~Panel();

    const char* const label;
    float y_pos = panel_y;
};

struct DisableGui
{
    DisableGui( bool disable = true );
    
    ~DisableGui();

    void operator()();

    int state; 
};

struct MouseEvent
{
    Vector2 position = { 0.0f, 0.0f };
    Vector2 last_mouse_position = { 0.0f, 0.0f };
    float scale = 1.0f;
    float last_click_time = 0.0f;
    bool dragging = false;
    bool double_click = false;
};

} // namespace gui {