
#ifndef _game_hpp_
#define _game_hpp_

void resize_view( float ww, float wh, float glvw, float glvh );
void set_mouse_position( float x, float y );
void set_mouse_state( unsigned int state );
void set_mouse_scroll_value( float scroll_value );

// void move_game_camera( float x, float y );

void init_game();
void input_game();
void update_game();
void render_game();

#endif