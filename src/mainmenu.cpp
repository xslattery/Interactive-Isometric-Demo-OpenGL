#include <OpenGL/gl3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <ft2build.h>
#include FT_FREETYPE_H

#include <vector>

#include "debug.hpp"
#include "text.hpp"
#include "shader.hpp"
#include "sprite.hpp"

#include "mainmenu.hpp"

extern glm::vec2 window_size;
extern glm::vec2 gl_viewport_size;
extern glm::vec2 render_dimensions;
extern glm::vec2 mouse_position;
extern unsigned int mouse_state;
extern FT_Library freeType;
extern float delta_time;
extern glm::mat4 viewMatrix;
extern glm::mat4 projectionMatrix;
extern glm::vec3 cameraPosition;
extern bool down_keys[256];

void Main_Menu::init() {
	tm_shader_id = LoadShaders( "res/shaders/textshader_vert.glsl", "res/shaders/textshader_frag.glsl" );
	pgt.fontsize = 32 ;
	create_packed_glyph_texture( pgt, "res/Pixel-UniCode.ttf", freeType, GL_NEAREST );

	tm.position = glm::vec3( render_dimensions.x/2 - 135, render_dimensions.y/2 - 190, 0 );
	tm.transform = glm::translate( glm::mat4(1), tm.position );	
	tm.fontsize = 130;
	create_text_mesh( "Untitled", tm, pgt, tm_shader_id );

	tm_play.position = glm::vec3( render_dimensions.x/2 - 25, render_dimensions.y/2 - 60, 1 );
	tm_play.transform = glm::translate( glm::mat4(1), tm_play.position );	
	tm_play.fontsize = 40;
	create_text_mesh( "Play", tm_play, pgt, tm_shader_id );

	tm_options.position = glm::vec3( render_dimensions.x/2 - 50, render_dimensions.y/2, 1 );
	tm_options.transform = glm::translate( glm::mat4(1), tm_options.position );	
	tm_options.fontsize = 40;
	create_text_mesh( "Settings", tm_options, pgt, tm_shader_id );

	ts_shader_id = LoadShaders( "res/shaders/spriteshader_textured_vert.glsl", "res/shaders/spriteshader_textured_frag.glsl" );
	LoadTexture( &ts_texture_id, "res/sprites/9slice_brown_border.png" );
	
	LoadTexture( &nsbb_texture_id_not_pressed, "res/sprites/9slice_button.png" );
	LoadTexture( &nsbb_texture_id_pressed, "res/sprites/9slice_button_upsidedown.png" );
	nsbb_texture_id_play = nsbb_texture_id_not_pressed;
	nsbb_texture_id_options = nsbb_texture_id_not_pressed;
	
	nine_sliced_sprite.transform_matrix = glm::translate( glm::mat4(1), glm::vec3(render_dimensions.x/2, render_dimensions.y/2 - 100, 0) );
	create_nine_sliced_sprite( nine_sliced_sprite, glm::vec2(200, 250), glm::vec2(8,8), glm::vec2(0.5f, 0.0f), ts_shader_id );

	nine_sliced_sprite_play.transform_matrix = glm::translate( glm::mat4(1), glm::vec3(render_dimensions.x/2, render_dimensions.y/2 - 70, 0.5f) );
	create_nine_sliced_sprite( nine_sliced_sprite_play, glm::vec2(150, 50), glm::vec2(8,8), glm::vec2(0.5f, 0.0f), ts_shader_id );

	nine_sliced_sprite_options.transform_matrix = glm::translate( glm::mat4(1), glm::vec3(render_dimensions.x/2, render_dimensions.y/2 - 15, 0.5f) );
	create_nine_sliced_sprite( nine_sliced_sprite_options, glm::vec2(150, 50), glm::vec2(8,8), glm::vec2(0.5f, 0.0f), ts_shader_id );

	LoadTexture( &border_texture_id, "res/sprites/9slice_brown_border_only.png" );
	border.transform_matrix = glm::translate( glm::mat4(1), glm::vec3(render_dimensions.x/2, render_dimensions.y/2, -1.0f) );
	create_nine_sliced_sprite( border, glm::vec2(render_dimensions.x-50, render_dimensions.y-50), glm::vec2(8,8), glm::vec2(0.5f), ts_shader_id );
}

void Main_Menu::input() {
	if ( main_menu_enabled ) {
		static bool m1_pressed_play = false;
		if ( mouse_state == 1 && mouse_position.x > render_dimensions.x/2 - 150/2 && mouse_position.x < render_dimensions.x/2 + 150/2 && mouse_position.y > render_dimensions.y/2 - 70 && mouse_position.y < render_dimensions.y/2 - 20 ) {
			play_button_color = glm::vec4(0.9f, 0.9f, 0.9f, 1.0f);
			nsbb_texture_id_play = nsbb_texture_id_pressed;
			tm_play.position = glm::vec3( render_dimensions.x/2 - 25, render_dimensions.y/2 - 57, 1 );
			tm_play.transform = glm::translate( glm::mat4(1), tm_play.position );
			m1_pressed_play = true;
		} else {
			if ( m1_pressed_play ) {
				if ( mouse_position.x > render_dimensions.x/2 - 150/2 && mouse_position.x < render_dimensions.x/2 + 150/2 && mouse_position.y > render_dimensions.y/2 - 70 && mouse_position.y < render_dimensions.y/2 - 20 ) exit_main_menu = true;
				m1_pressed_play = false;
			}
			play_button_color = glm::vec4(1,1,1,1);
			nsbb_texture_id_play = nsbb_texture_id_not_pressed;
			tm_play.position = glm::vec3( render_dimensions.x/2 - 25, render_dimensions.y/2 - 60, 1 );
			tm_play.transform = glm::translate( glm::mat4(1), tm_play.position );
		}

		static bool m1_pressed_options = false;
		if ( mouse_state == 1 && mouse_position.x > render_dimensions.x/2 - 150/2 && mouse_position.x < render_dimensions.x/2 + 150/2 && mouse_position.y > render_dimensions.y/2 - 10 && mouse_position.y < render_dimensions.y/2 + 40 ) {
			if ( mouse_state == 1 ) options_button_color = glm::vec4(0.9f, 0.9f, 0.9f, 1.0f);
			nsbb_texture_id_options = nsbb_texture_id_pressed;
			tm_options.position = glm::vec3( render_dimensions.x/2 - 50, render_dimensions.y/2 + 3, 1 );
			tm_options.transform = glm::translate( glm::mat4(1), tm_options.position );
			m1_pressed_options = true;
		} else {
			if ( m1_pressed_options ) {
				if ( mouse_position.x > render_dimensions.x/2 - 150/2 && mouse_position.x < render_dimensions.x/2 + 150/2 && mouse_position.y > render_dimensions.y/2 - 10 && mouse_position.y < render_dimensions.y/2 + 40 ) exit_main_menu = true;
				m1_pressed_options = false;
			}
			options_button_color = glm::vec4(1,1,1,1);
			nsbb_texture_id_options = nsbb_texture_id_not_pressed;
			tm_options.position = glm::vec3( render_dimensions.x/2 - 50, render_dimensions.y/2, 1 );
			tm_options.transform = glm::translate( glm::mat4(1), tm_options.position );
		}
	}
}

void Main_Menu::update() {
	if ( main_menu_enabled ) {
		static glm::vec2 old_render_dimensions;
		if ( old_render_dimensions != render_dimensions ) {
			old_render_dimensions = render_dimensions;

			nine_sliced_sprite.transform_matrix = glm::translate( glm::mat4(1), glm::vec3(render_dimensions.x/2, render_dimensions.y/2-100, 0) );
			nine_sliced_sprite_play.transform_matrix = glm::translate( glm::mat4(1), glm::vec3(render_dimensions.x/2, render_dimensions.y/2 - 70, 0.5f) );
			nine_sliced_sprite_options.transform_matrix = glm::translate( glm::mat4(1), glm::vec3(render_dimensions.x/2, render_dimensions.y/2 - 8, 0.5f) );

			border.transform_matrix = glm::translate( glm::mat4(1), glm::vec3(render_dimensions.x/2, render_dimensions.y/2, -1.0f) );
			create_nine_sliced_sprite( border, glm::vec2(render_dimensions.x-5, render_dimensions.y-5), glm::vec2(8,8), glm::vec2(0.5f), ts_shader_id );
			
			tm.position = glm::vec3( render_dimensions.x/2 - 150, render_dimensions.y/2 - 210, 0 );
			tm.transform = glm::translate( glm::mat4(1), tm.position );	

			tm_play.position = glm::vec3( render_dimensions.x/2 - 25, render_dimensions.y/2 - 60, 1 );
			tm_play.transform = glm::translate( glm::mat4(1), tm_play.position );

			tm_options.position = glm::vec3( render_dimensions.x/2 - 50, render_dimensions.y/2, 1 );
			tm_options.transform = glm::translate( glm::mat4(1), tm_options.position );
		}

		if ( exit_main_menu ) {
			exit_countdown -= delta_time;
			if ( exit_countdown <= 0) {
				main_menu_enabled = false;
			}
		}
	}
}

void Main_Menu::render() {
	if ( main_menu_enabled ) {
		glUseProgram( ts_shader_id );

			setUniformMat4( ts_shader_id, "view", viewMatrix );
			setUniformMat4( ts_shader_id, "projection", projectionMatrix );

			setUniform4f( ts_shader_id, "tintColor", glm::vec4(1.0f) );
			render_nine_sliced_sprite( border, ts_shader_id, border_texture_id );
			render_nine_sliced_sprite( nine_sliced_sprite, ts_shader_id, ts_texture_id );

			setUniform4f( ts_shader_id, "tintColor", play_button_color );
			render_nine_sliced_sprite( nine_sliced_sprite_play, ts_shader_id, nsbb_texture_id_play );

			setUniform4f( ts_shader_id, "tintColor", options_button_color );
			render_nine_sliced_sprite( nine_sliced_sprite_options, ts_shader_id, nsbb_texture_id_options );


		glUseProgram( tm_shader_id );
			
			setUniformMat4( tm_shader_id, "view", viewMatrix );
			setUniformMat4( tm_shader_id, "projection", projectionMatrix );
			
			setUniform4f( tm_shader_id, "overlayColor", glm::vec4(1.0f) );
			render_text_mesh( tm, tm_shader_id );

			setUniform4f( tm_shader_id, "overlayColor", play_button_color );
			render_text_mesh( tm_play, tm_shader_id );

			setUniform4f( tm_shader_id, "overlayColor", options_button_color );
			render_text_mesh( tm_options, tm_shader_id );

		glUseProgram( 0 );
	}
}