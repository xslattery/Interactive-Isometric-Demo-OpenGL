#include <OpenGL/gl3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <mach/mach_time.h>

#include <vector>
#include <string>

#include "debug.hpp"
#include "text.hpp"
#include "shader.hpp"
#include "sprite.hpp"
#include "mainmenu.hpp"
#include "perlin.hpp"
#include "simplex.hpp"

enum Tile_Type {
	AIR = 0,

	DIRT = 1,
	DIRT_RAMP = 2,

	STONE = 3,

	WOOD = 4,
	WOOD_RAMP = 5,

	LAVA = 6,
};

enum Direction {
	NONE = 0,
	XP = 1,
	XN = 2,
	ZP = 3,
	ZN = 4,

	XP_ZP = 5,
	XN_ZN = 6,
	XP_ZN = 7,
	XN_ZP = 8,
};

struct Tile {
	Tile_Type type = AIR;
	Direction direction = NONE;
	bool is_ramp = false;
	bool is_full = false;
};

struct World {
	static const int SIZE_X = 128;
	static const int SIZE_Z = 128;
	static const int SIZE_Y = 128;
	Tile tiles[SIZE_Y][SIZE_Z][SIZE_X];

	TexturedSpriteBatch tile_sb[SIZE_Y];
	bool generated_full_sb[SIZE_Y];

	unsigned int shaderID = 0;
	unsigned int texID = 0;
};

extern void refresh_after_resize();
extern void hide_cursor();

// Global Variables
glm::vec2 window_size;
glm::vec2 gl_viewport_size;
bool dynamic_resolution = true;
glm::vec2 render_dimensions( 1280, 800 );

glm::vec2 mouse_position;
unsigned int mouse_state;
float mouse_scroll;

FT_Library freeType;
float delta_time = 0;
glm::mat4 viewMatrix;
glm::mat4 projectionMatrix;
glm::vec3 cameraPosition;

glm::mat4 game_viewMatrix;
glm::mat4 game_projectionMatrix;
glm::vec3 game_cameraPosition;
float game_camera_scale = 50;

bool down_keys[256];

/////////////////////
/////////////

static Main_Menu main_menu;

static Packed_Glyph_Texture debug_pgt;
static Text_Mesh debug_text_mesh = {0};
static unsigned int debug_text_shader_id;

static World world;
static unsigned int world_cutoff_height = World::SIZE_Y;

static TexturedSpriteBatch cursor_sb;
static unsigned int half_height_texture = 0;
static bool render_half_height = false;

static bool cursor_disable_depth = false;

//////////////////////
/////////////
/////////////////

void resize_view( float ww, float wh, float glvw, float glvh );

static float generateHeightmap ( float xx, float zz, float scale, int octaves, float persistance, float lacunarity, bool power ) {

	if ( scale <= 0 ) scale = 0.0001f;
	if ( octaves < 1 ) octaves = 1;
	if ( persistance > 1 ) persistance = 1;
	if ( persistance < 0 ) persistance = 0;
	if ( lacunarity < 1 ) lacunarity = 1;

	float amplitude = 1.0f;
	float frequency = 1.0f;
	float noiseValue = 0.0f;

	for ( int i = 0; i < octaves; ++i ) {

		float sampleX = xx / scale * frequency;
		float sampleZ = zz / scale * frequency;

		// float nv = pow(2.71828182845904523536, noise_2d(sampleX, sampleZ));
		float nv = noise_2d(sampleX, sampleZ);

		noiseValue += nv * amplitude;

		amplitude *= persistance;
		frequency *= lacunarity;

	}

	if ( power ) noiseValue = pow(2.71828182845904523536, noiseValue);

	return noiseValue; 

}

static void generate_world_mesh_layer ( int layer, bool occlude = false ) {
	
	world.generated_full_sb[layer] = !occlude;

	glm::vec2 x_vector = glm::vec2(0.5f, -0.25f);
	glm::vec2 z_vector = glm::vec2(-0.5f, -0.25f);
	glm::vec2 y_vector = glm::vec2(0, -1);

	int y = layer;
	prepairTexturedSpriteBatchForPush( &world.tile_sb[y] ); 
	for (int z = 0; z < World::SIZE_Z; ++z) {
		for (int x = 0; x < World::SIZE_X; ++x) {

			// This is testing to see if we can skip rendering this
			// tile because it is obstructed by other tiles.
			if ( occlude ) 
				if ( x > 0 && world.tiles[y][z][x-1].type != AIR && !world.tiles[y][z][x-1].is_ramp ) 
					if ( z > 0 && world.tiles[y][z-1][x].type != AIR && !world.tiles[y][z-1][x].is_ramp ) 
						if ( y < World::SIZE_Y-1 && world.tiles[y+1][z][x].type != AIR )
							continue;

			glm::vec2 loc = (float)x*x_vector*32.0f + (float)z*z_vector*32.0f + (float)y*y_vector*16.0f;
			glm::vec4 tex = glm::vec4(0, 0, 1.0f, 1.0f);

			switch ( world.tiles[y][z][x].type ) {
				case AIR: continue; break;
				case DIRT: tex = glm::vec4(0, 0, 0.125f, 0.125f); break;
				case DIRT_RAMP: {
					switch ( world.tiles[y][z][x].direction ) {
						case XP_ZP: tex = glm::vec4(0.625f, 0.0f, 0.750f, 0.125f); break;
						case XN_ZN: tex = glm::vec4(0.875f, 0.125f, 1.000f, 0.250f); break;
						case XP_ZN: tex = glm::vec4(0.750f, 0.125f, 0.875f, 0.250f); break;
						case XN_ZP: tex = glm::vec4(0.625f, 0.125f, 0.750f, 0.250f); break;
						
						// case XP: tex = glm::vec4(0.125f, 0.500f, 0.250f, 0.625f); break;
						// case ZP: tex = glm::vec4(0.250f, 0.500f, 0.375f, 0.625f); break;
						// case ZN: tex = glm::vec4(0.375f, 0.500f, 0.500f, 0.625f); break;
						// case XN: tex = glm::vec4(0.500f, 0.500f, 0.625f, 0.625f); break;
						case XP: tex = glm::vec4(0.375f, 0.0f, 0.500f, 0.125f); break;
						case ZP: tex = glm::vec4(0.500f, 0.0f, 0.625f, 0.125f); break;
						case XN: tex = glm::vec4(0.875f, 0.0f, 1.000f, 0.125f); break;
						case ZN: tex = glm::vec4(0.750f, 0.0f, 0.875f, 0.125f); break;
						default: break;
					}
				} break;
				case WOOD_RAMP: {
					switch ( world.tiles[y][z][x].direction ) {
						case XP: tex = glm::vec4(0.375f, 0.0f, 0.500f, 0.125f); break;
						case ZP: tex = glm::vec4(0.500f, 0.0f, 0.625f, 0.125f); break;
						case XN: tex = glm::vec4(0.875f, 0.0f, 1.000f, 0.125f); break;
						case ZN: tex = glm::vec4(0.750f, 0.0f, 0.875f, 0.125f); break;
						default: break;
					}
				} break;
				case STONE: tex = glm::vec4(0, 0.250f, 0.125f, 0.375f); break;
				case WOOD: tex = glm::vec4(0, 0.500f, 0.125f, 0.625f); break;
				case LAVA: tex = glm::vec4(0.000f, 0.750f, 0.125f, 0.875f); break;
				default: tex = glm::vec4(0, 0, 1.0f, 1.0f); break;
			}
			
			auto is_empty = [&]( int yy, int zz, int xx ) -> bool {
				if ( zz < World::SIZE_Z && xx < World::SIZE_X && yy < World::SIZE_Y ) {
					if ( zz >= 0 && xx >= 0 && yy >= 0) { return world.tiles[yy][zz][xx].type == AIR; }
					else { return true; }
				} else { return true; }
			};

			auto is_ramp = [&]( int yy, int zz, int xx ) -> bool {
				if ( zz < World::SIZE_Z && xx < World::SIZE_X && yy < World::SIZE_Y ) {
					if ( zz >= 0 && xx >= 0 && yy >= 0) { return world.tiles[yy][zz][xx].is_ramp; }
					else { return false; }
				} else { return false; }
			};

			auto is_surrounded = [&]( int yy, int zz, int xx ) -> bool {
				if ( zz < World::SIZE_Z && xx < World::SIZE_X && yy < World::SIZE_Y ) {
					if ( zz >= 0 && xx >= 0 && yy >= 0) { return world.tiles[yy][zz][xx].type == AIR || world.tiles[yy][zz][xx].type == LAVA; }
					else { return false; }
				} else { return false; }
			};

			if ( !is_surrounded(y+1, z, x) && !is_surrounded(y-1, z, x) ) {
				if ( !is_surrounded(y, z+1, x) && !is_ramp(y, z+1, x) ) {
					if ( !is_surrounded(y, z-1, x) && !is_ramp(y, z-1, x) ) {
						if ( !is_surrounded(y, z, x+1) && !is_ramp(y, z, x+1) ) {
							if ( !is_surrounded(y, z, x-1) && !is_ramp(y, z, x-1) ) {
								tex = glm::vec4( 0.875f, 0.875f, 1.0f, 1.0f );
							}
						}
					}
				}
			}

			pushToTexturedSpriteBatch( &world.tile_sb[y], glm::vec3(loc.x, loc.y, -(x + z) + y*2 ), glm::vec2(1), 0, glm::vec2(32, 32), glm::vec2(0.5f, 1.0f), tex, 1.0f );

			if ( world.tiles[y][z][x].is_full ) {
				if ( is_empty(y, z, x+1) || is_ramp(y, z, x+1) ) { pushToTexturedSpriteBatch( &world.tile_sb[y], glm::vec3(loc.x, loc.y, -(x + z) + y*2 + 0.1f ), glm::vec2(1), 0, glm::vec2(32, 32), glm::vec2(0.5f, 1.0f), glm::vec4(0.250f ,0.0f, 0.375f, 0.125f), 1.0f ); }
				if ( is_empty(y, z+1, x) || is_ramp(y, z+1, x) )  { pushToTexturedSpriteBatch( &world.tile_sb[y], glm::vec3(loc.x, loc.y, -(x + z) + y*2 + 0.1f ), glm::vec2(1), 0, glm::vec2(32, 32), glm::vec2(0.5f, 1.0f), glm::vec4(0.125f, 0.0f, 0.250f, 0.125f), 1.0f ); }
				if ( is_empty(y-1, z, x) ) { 
					pushToTexturedSpriteBatch( &world.tile_sb[y], glm::vec3(loc.x, loc.y, -(x + z) + y*2 + 0.1f ), glm::vec2(1), 0, glm::vec2(32, 32), glm::vec2(0.5f, 1.0f), glm::vec4(0.625f, 0.375f, 0.750f, 0.500f), 1.0f );
					pushToTexturedSpriteBatch( &world.tile_sb[y], glm::vec3(loc.x, loc.y, -(x + z) + y*2 + 0.1f ), glm::vec2(1), 0, glm::vec2(32, 32), glm::vec2(0.5f, 1.0f), glm::vec4(0.750f, 0.375f, 0.875f, 0.500f), 1.0f );
				}
			}

			if ( world.tiles[y][z][x].is_ramp ) {
				if 		( world.tiles[y][z][x].direction == XP_ZP ) { }
				else if ( world.tiles[y][z][x].direction == XN_ZN ) { }
				else if ( world.tiles[y][z][x].direction == XP_ZN ) { pushToTexturedSpriteBatch( &world.tile_sb[y], glm::vec3(loc.x, loc.y, -(x + z) + y*2 + 0.1f ), glm::vec2(1), 0, glm::vec2(32, 32), glm::vec2(0.5f, 1.0f), glm::vec4(0.375f, 0.375f, 0.500f, 0.500f), 1.0f ); }
				else if ( world.tiles[y][z][x].direction == XN_ZP ) { pushToTexturedSpriteBatch( &world.tile_sb[y], glm::vec3(loc.x, loc.y, -(x + z) + y*2 + 0.1f ), glm::vec2(1), 0, glm::vec2(32, 32), glm::vec2(0.5f, 1.0f), glm::vec4(0.500f, 0.375f, 0.625f, 0.500f), 1.0f ); }
				else if ( world.tiles[y][z][x].direction == XP && is_empty(y, z+1, x) ) { pushToTexturedSpriteBatch( &world.tile_sb[y], glm::vec3(loc.x, loc.y, -(x + z) + y*2 + 0.1f ), glm::vec2(1), 0, glm::vec2(32, 32), glm::vec2(0.5f, 1.0f), glm::vec4(0.375f, 0.125f, 0.500f, 0.250f), 1.0f ); } 
				else if ( world.tiles[y][z][x].direction == ZP && is_empty(y, z, x+1) ) { pushToTexturedSpriteBatch( &world.tile_sb[y], glm::vec3(loc.x, loc.y, -(x + z) + y*2 + 0.1f ), glm::vec2(1), 0, glm::vec2(32, 32), glm::vec2(0.5f, 1.0f), glm::vec4(0.500f, 0.125f, 0.625f, 0.250f), 1.0f ); }
				else if ( world.tiles[y][z][x].direction == XN && is_empty(y, z-1, x) ) { }
				else if ( world.tiles[y][z][x].direction == ZN && is_empty(y, z, x-1) ) { }
			}

		}
	}
	buildTexturedSpriteBatch( &world.tile_sb[y], world.shaderID );
}

static void generate_world_mesh () {

	for (int y = 0; y < World::SIZE_Y; ++y) {
		generate_world_mesh_layer(y, true);
	}

}

void generate_world() {

	for (int y = 0; y < World::SIZE_Y; ++y) {
		for (int z = 0; z < World::SIZE_Z; ++z) {
			for (int x = 0; x < World::SIZE_X; ++x) {
				
				world.tiles[y][z][x].type = AIR;

				int height = (int)(generateHeightmap( x, World::SIZE_Z-z, 350, 4, 0.5f, 2.5f, 1 ) * 25.0f ) + 64;
				if ( height > y ) {
					world.tiles[y][z][x].type = STONE;
					world.tiles[y][z][x].is_full = true;
				}
				else if ( height == y ) {
					world.tiles[y][z][x].type = DIRT;
					world.tiles[y][z][x].is_full = true;
				}

				float sim_val = simplex_noise( 3, x/32.0f, y/32.0f, z/32.0f );
				// if ( y > 100 ) sim_val += (y-128);
				if ( y > 32 ) sim_val = 1000;
				if ( sim_val < 2.1f ) {
					world.tiles[y][z][x].type = LAVA;
					world.tiles[y][z][x].is_full = false;
				}

			}
		}
	}

	auto is_empty = [&]( int yy, int zz, int xx ) -> bool {
		if ( zz < World::SIZE_Z && xx < World::SIZE_X && yy < World::SIZE_Y ) {
			if ( zz >= 0 && xx >= 0 && yy >= 0) { return world.tiles[yy][zz][xx].type == AIR; }
			else { return true; }
		} else { return true; }
	};

	auto is_ramp = [&]( int yy, int zz, int xx ) -> bool {
		if ( zz < World::SIZE_Z && xx < World::SIZE_X && yy < World::SIZE_Y ) {
			if ( zz >= 0 && xx >= 0 && yy >= 0) { return world.tiles[yy][zz][xx].is_ramp; }
			else { return false; }
		} else { return false; }
	};

	auto get_tile = [&]( int yy, int zz, int xx ) -> Tile {
		if ( zz < World::SIZE_Z && xx < World::SIZE_X && yy < World::SIZE_Y ) {
			if ( zz >= 0 && xx >= 0 && yy >= 0) { return world.tiles[yy][zz][xx]; }
			else { return {}; }
		} else { return {}; }
	};

	for (int y = 0; y < World::SIZE_Y; ++y) {
		for (int z = 0; z < World::SIZE_Z; ++z) {
			for (int x = 0; x < World::SIZE_X; ++x) {

				if ( is_empty(y, z, x) && is_empty(y+1, z, x) && !is_empty(y-1, z, x) && !is_ramp(y-1, z, x) ) {

					if ( get_tile(y, z, x+1).type == DIRT || get_tile(y, z, x-1).type == DIRT || get_tile(y, z+1, x).type == DIRT || get_tile(y, z-1, x).type == DIRT) {
						
						world.tiles[y][z][x].type = DIRT_RAMP;
						world.tiles[y][z][x].is_ramp = true;
						
						if ( get_tile(y, z, x+1).is_full && get_tile(y, z+1, x).is_full && get_tile(y, z, x-1).is_full ) 		{ world.tiles[y][z][x].direction = ZP; }
						else if ( get_tile(y, z, x+1).is_full && get_tile(y, z+1, x).is_full && get_tile(y, z-1, x).is_full ) 	{ world.tiles[y][z][x].direction = XP; }
						else if ( get_tile(y, z, x-1).is_full && get_tile(y, z+1, x).is_full && get_tile(y, z-1, x).is_full ) 	{ world.tiles[y][z][x].direction = XN; }
						else if ( get_tile(y, z, x+1).is_full && get_tile(y, z-1, x).is_full && get_tile(y, z, x-1).is_full ) 	{ world.tiles[y][z][x].direction = ZN; }
						else if ( get_tile(y, z, x+1).is_full && get_tile(y, z+1, x).is_full) 									{ world.tiles[y][z][x].direction = XP_ZP; }
						else if ( get_tile(y, z, x-1).is_full && get_tile(y, z-1, x).is_full) 									{ world.tiles[y][z][x].direction = XN_ZN; }
						else if ( get_tile(y, z, x-1).is_full && get_tile(y, z+1, x).is_full) 									{ world.tiles[y][z][x].direction = XN_ZP; }
						else if ( get_tile(y, z, x+1).is_full && get_tile(y, z-1, x).is_full) 									{ world.tiles[y][z][x].direction = XP_ZN; }
						else if ( get_tile(y, z, x+1).is_full ) 																{ world.tiles[y][z][x].direction = XP; }
						else if ( get_tile(y, z+1, x).is_full ) 																{ world.tiles[y][z][x].direction = ZP; }
						else if ( get_tile(y, z, x-1).is_full ) 																{ world.tiles[y][z][x].direction = XN; }
						else if ( get_tile(y, z-1, x).is_full ) 																{ world.tiles[y][z][x].direction = ZN; }
					}
				}

			}
		}
	}

}

void init_game() {

	if ( dynamic_resolution ) { render_dimensions = window_size; }

	FT_Init_FreeType( &freeType );
	
	glEnable( GL_DEPTH_TEST );

	glCullFace( GL_BACK );	
	glEnable( GL_CULL_FACE );

	glEnable( GL_BLEND);
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ZERO);
	
	glClearColor(1, 0, 1, 1);

	cameraPosition = glm::vec3(0, 0, 100);
	viewMatrix = glm::translate( glm::mat4(1), -cameraPosition ); 
	projectionMatrix = glm::ortho( 0.0f, render_dimensions.x, render_dimensions.y, 0.0f, 0.1f, 1000.0f);

	game_cameraPosition = glm::vec3(0, -(World::SIZE_X+2)*10, 1500);
	game_camera_scale = 100.0f;
	game_viewMatrix = glm::translate( glm::scale(glm::mat4(1), glm::vec3(1.0f/game_camera_scale, 1.0f/game_camera_scale, 1)), -game_cameraPosition ); 
	glm::vec2 aspect = glm::vec2( (float)render_dimensions.x/render_dimensions.y*10, (float)render_dimensions.x/render_dimensions.y*render_dimensions.y/render_dimensions.x*10 );
	game_projectionMatrix = glm::ortho( -aspect.x/2, aspect.x/2, aspect.y/2, -aspect.y/2, 0.1f, 2000.0f);

	debug_text_shader_id = LoadShaders( "res/shaders/textshader_vert.glsl", "res/shaders/textshader_frag.glsl" );
	debug_pgt.fontsize = 32 ;
	create_packed_glyph_texture( debug_pgt, "res/Menlo-Regular.ttf", freeType );
	debug_text_mesh.position = glm::vec3( 15, 15, 0 );
	debug_text_mesh.transform = glm::translate( glm::mat4(1), debug_text_mesh.position );	
	debug_text_mesh.fontsize = 16;
	create_text_mesh( "dt: ", debug_text_mesh, debug_pgt, debug_text_shader_id );

	main_menu.init();

	world.shaderID = LoadShaders( "res/shaders/spritebatchshader_texture_vert.glsl", "res/shaders/spritebatchshader_texture_frag.glsl" );
	LoadTexture( &world.texID, "res/sprites/TileMap.png" );
	LoadTexture( &half_height_texture, "res/sprites/TileMapHalfHeight.png" );
	

	cursor_sb.shaderID = LoadShaders( "res/shaders/spritebatchshader_texture_vert.glsl", "res/shaders/spritebatchshader_texture_frag.glsl" );
	LoadTexture( &cursor_sb.texID, "res/sprites/TileMap.png" );

	generate_world();
	generate_world_mesh();

}

void input_game() {

	game_camera_scale += mouse_scroll * delta_time;
	if ( game_camera_scale < 0.01f ) game_camera_scale = 0.01f;
	game_viewMatrix = glm::translate( glm::scale(glm::mat4(1), glm::vec3(1.0f/game_camera_scale, 1.0f/game_camera_scale, 1)), -game_cameraPosition ); 

	auto apply_resoultion = [&]() {
		projectionMatrix = glm::ortho( 0.0f, render_dimensions.x, render_dimensions.y, 0.0f, 0.1f, 1000.0f);
		glm::vec2 aspect = glm::vec2( (float)render_dimensions.x/render_dimensions.y*10, (float)render_dimensions.x/render_dimensions.y*render_dimensions.y/render_dimensions.x*10 );
		game_projectionMatrix = glm::ortho( -aspect.x/2, aspect.x/2, aspect.y/2, -aspect.y/2, 0.1f, 2000.0f);
		refresh_after_resize();
	};

	if ( down_keys['0'] ) {
		dynamic_resolution = true;
		resize_view( window_size.x, window_size.y, gl_viewport_size.x, gl_viewport_size.y );
		// projectionMatrix = glm::ortho( 0.0f, render_dimensions.x, render_dimensions.y, 0.0f, 0.1f, 1000.0f);
		// glm::vec2 aspect = glm::vec2( (float)render_dimensions.x/render_dimensions.y*10, (float)render_dimensions.x/render_dimensions.y*render_dimensions.y/render_dimensions.x*10 );
		// game_projectionMatrix = glm::ortho( -aspect.x/2, aspect.x/2, aspect.y/2, -aspect.y/2, 0.1f, 2000.0f);
		refresh_after_resize();
	}
	else if ( down_keys['1'] ) {
		dynamic_resolution = false;
		render_dimensions = glm::vec2( 800, 600 );
		apply_resoultion();
	}
	else if ( down_keys['2'] ) {
		dynamic_resolution = false;
		render_dimensions = glm::vec2( 1024, 768 );
		apply_resoultion();
	}
	else if ( down_keys['3'] ) {
		dynamic_resolution = false;
		render_dimensions = glm::vec2( 1280, 720 );
		apply_resoultion();
	}
	else if ( down_keys['4'] ) {
		dynamic_resolution = false;
		render_dimensions = glm::vec2( 1366, 768 );
		apply_resoultion();
	}
	else if ( down_keys['5'] ) {
		dynamic_resolution = false;
		render_dimensions = glm::vec2( 1440, 900 );
		apply_resoultion();
	}
	else if ( down_keys['6'] ) {
		dynamic_resolution = false;
		render_dimensions = glm::vec2( 1600, 900 );
		apply_resoultion();
	}
	else if ( down_keys['7'] ) {
		dynamic_resolution = false;
		render_dimensions = glm::vec2( 1680, 1050 );
		apply_resoultion();
	}
	else if ( down_keys['8'] ) {
		dynamic_resolution = false;
		render_dimensions = glm::vec2( 1920, 1200 );
		apply_resoultion();
	}
	else if ( down_keys['9'] ) {
		dynamic_resolution = false;
		render_dimensions = glm::vec2( 2560, 1440 );
		apply_resoultion();
	}
	
	if ( !main_menu.main_menu_enabled ) {
		static bool hit_once = false;
		if ( !hit_once ) { hide_cursor(); hit_once = true; }

		if ( down_keys['w'] ) {
			game_cameraPosition += glm::vec3(0, -1, 0)*delta_time*600.0f;
			game_viewMatrix = glm::translate( glm::scale(glm::mat4(1), glm::vec3(1.0f/game_camera_scale, 1.0f/game_camera_scale, 1)), -game_cameraPosition ); 
		}
		if ( down_keys['s'] ) {
			game_cameraPosition += glm::vec3(0, 1, 0)*delta_time*600.0f;
			game_viewMatrix = glm::translate( glm::scale(glm::mat4(1), glm::vec3(1.0f/game_camera_scale, 1.0f/game_camera_scale, 1)), -game_cameraPosition ); 
		}
		if ( down_keys['a'] ) {
			game_cameraPosition += glm::vec3(-1, 0, 0)*delta_time*600.0f;
			game_viewMatrix = glm::translate( glm::scale(glm::mat4(1), glm::vec3(1.0f/game_camera_scale, 1.0f/game_camera_scale, 1)), -game_cameraPosition ); 
		}
		if ( down_keys['d'] ) {
			game_cameraPosition += glm::vec3(1, 0, 0)*delta_time*600.0f;
			game_viewMatrix = glm::translate( glm::scale(glm::mat4(1), glm::vec3(1.0f/game_camera_scale, 1.0f/game_camera_scale, 1)), -game_cameraPosition ); 
		}

		glm::vec2 aspect = glm::vec2( (float)render_dimensions.x/render_dimensions.y*10, (float)render_dimensions.x/render_dimensions.y*render_dimensions.y/render_dimensions.x*10 );
		float gmp_x = 1.0f/render_dimensions.x*mouse_position.x*aspect.x*game_camera_scale-aspect.x*game_camera_scale/2+game_cameraPosition.x;
		float gmp_y = 1.0f/render_dimensions.y*mouse_position.y*aspect.y*game_camera_scale-aspect.y*game_camera_scale/2+game_cameraPosition.y;// + 16.0f;
		glm::vec2 x_vector = glm::vec2(0.5f, -0.25f);
		glm::vec2 z_vector = glm::vec2(-0.5f, -0.25f);
		float mouse_grid_z = -ceil((gmp_y / 16.0f) + (gmp_x / 32.0f));
		float mouse_grid_x = -ceil((-gmp_x / 32.0f) + (gmp_y / 16.0f));
		glm::vec2 gPos = mouse_grid_x*x_vector*32.0f + mouse_grid_z*z_vector*32.0f; //* (x_vector*32.0f) * (z_vector*32.0f) * (y_vector*16.0f);

		static int block_to_place = 0;

		static bool n_pressed = false;
		if ( down_keys['n'] ) { if( !n_pressed ) { block_to_place++; } n_pressed = true; }
		else n_pressed = false;
		
		if ( block_to_place > 5 ) block_to_place = 0;

		if ( block_to_place == 0 ) cursor_disable_depth = false;
		else cursor_disable_depth = true;

		if ( mouse_state == 1 ) {

			if ( mouse_grid_x-world_cutoff_height+1 < World::SIZE_X && mouse_grid_z-world_cutoff_height+1 < World::SIZE_Z && mouse_grid_x-world_cutoff_height+1 >= 0 && mouse_grid_z-world_cutoff_height+1 >= 0 ) {
				if ( block_to_place == 0 ) {
					world.tiles[world_cutoff_height-1][(int)mouse_grid_z-world_cutoff_height+1][(int)mouse_grid_x-world_cutoff_height+1].type = AIR;
					world.tiles[world_cutoff_height-1][(int)mouse_grid_z-world_cutoff_height+1][(int)mouse_grid_x-world_cutoff_height+1].direction = NONE;
					world.tiles[world_cutoff_height-1][(int)mouse_grid_z-world_cutoff_height+1][(int)mouse_grid_x-world_cutoff_height+1].is_full = false;
					world.tiles[world_cutoff_height-1][(int)mouse_grid_z-world_cutoff_height+1][(int)mouse_grid_x-world_cutoff_height+1].is_ramp = false;
				} else if ( block_to_place == 1 ) {
					world.tiles[world_cutoff_height-1][(int)mouse_grid_z-world_cutoff_height+1][(int)mouse_grid_x-world_cutoff_height+1].type = WOOD;
					world.tiles[world_cutoff_height-1][(int)mouse_grid_z-world_cutoff_height+1][(int)mouse_grid_x-world_cutoff_height+1].direction = NONE;
					world.tiles[world_cutoff_height-1][(int)mouse_grid_z-world_cutoff_height+1][(int)mouse_grid_x-world_cutoff_height+1].is_full = true;
					world.tiles[world_cutoff_height-1][(int)mouse_grid_z-world_cutoff_height+1][(int)mouse_grid_x-world_cutoff_height+1].is_ramp = false;
				} 


				else if ( block_to_place == 2 ) {
					world.tiles[world_cutoff_height-1][(int)mouse_grid_z-world_cutoff_height+1][(int)mouse_grid_x-world_cutoff_height+1].type = WOOD_RAMP;
					world.tiles[world_cutoff_height-1][(int)mouse_grid_z-world_cutoff_height+1][(int)mouse_grid_x-world_cutoff_height+1].direction = XP;
					world.tiles[world_cutoff_height-1][(int)mouse_grid_z-world_cutoff_height+1][(int)mouse_grid_x-world_cutoff_height+1].is_full = false;
					world.tiles[world_cutoff_height-1][(int)mouse_grid_z-world_cutoff_height+1][(int)mouse_grid_x-world_cutoff_height+1].is_ramp = true;
				} else if ( block_to_place == 3 ) {
					world.tiles[world_cutoff_height-1][(int)mouse_grid_z-world_cutoff_height+1][(int)mouse_grid_x-world_cutoff_height+1].type = WOOD_RAMP;
					world.tiles[world_cutoff_height-1][(int)mouse_grid_z-world_cutoff_height+1][(int)mouse_grid_x-world_cutoff_height+1].direction = ZP;
					world.tiles[world_cutoff_height-1][(int)mouse_grid_z-world_cutoff_height+1][(int)mouse_grid_x-world_cutoff_height+1].is_full = false;
					world.tiles[world_cutoff_height-1][(int)mouse_grid_z-world_cutoff_height+1][(int)mouse_grid_x-world_cutoff_height+1].is_ramp = true;
				} else if ( block_to_place == 4 ) {
					world.tiles[world_cutoff_height-1][(int)mouse_grid_z-world_cutoff_height+1][(int)mouse_grid_x-world_cutoff_height+1].type = WOOD_RAMP;
					world.tiles[world_cutoff_height-1][(int)mouse_grid_z-world_cutoff_height+1][(int)mouse_grid_x-world_cutoff_height+1].direction = ZN;
					world.tiles[world_cutoff_height-1][(int)mouse_grid_z-world_cutoff_height+1][(int)mouse_grid_x-world_cutoff_height+1].is_full = false;
					world.tiles[world_cutoff_height-1][(int)mouse_grid_z-world_cutoff_height+1][(int)mouse_grid_x-world_cutoff_height+1].is_ramp = true;
				} else if ( block_to_place == 5 ) {
					world.tiles[world_cutoff_height-1][(int)mouse_grid_z-world_cutoff_height+1][(int)mouse_grid_x-world_cutoff_height+1].type = WOOD_RAMP;
					world.tiles[world_cutoff_height-1][(int)mouse_grid_z-world_cutoff_height+1][(int)mouse_grid_x-world_cutoff_height+1].direction = XN;
					world.tiles[world_cutoff_height-1][(int)mouse_grid_z-world_cutoff_height+1][(int)mouse_grid_x-world_cutoff_height+1].is_full = false;
					world.tiles[world_cutoff_height-1][(int)mouse_grid_z-world_cutoff_height+1][(int)mouse_grid_x-world_cutoff_height+1].is_ramp = true;
				}
				
				generate_world_mesh_layer( world_cutoff_height-1 );
				// if ( !world.generated_full_sb[world_cutoff_height-2] ){
					generate_world_mesh_layer( world_cutoff_height-2, true );
				// } 
			}

		}

		prepairTexturedSpriteBatchForPush( &cursor_sb ); 
			if ( block_to_place == 0 ) pushToTexturedSpriteBatch( &cursor_sb, glm::vec3(gPos.x, gPos.y, -(mouse_grid_x-world_cutoff_height+1 + mouse_grid_z-world_cutoff_height+1) + (world_cutoff_height-1)*2 + 0.5f ), glm::vec2(1), 0, glm::vec2(32, 32), glm::vec2(0.5f, 1.0f), glm::vec4(0.250f ,0.125f, 0.375f, 0.250f), 1.0f );
			if ( block_to_place == 1 ) pushToTexturedSpriteBatch( &cursor_sb, glm::vec3(gPos.x, gPos.y, -(mouse_grid_x-world_cutoff_height+1 + mouse_grid_z-world_cutoff_height+1) + (world_cutoff_height-1)*2 + 0.5f ), glm::vec2(1), 0, glm::vec2(32, 32), glm::vec2(0.5f, 1.0f), glm::vec4(0, 0.500f, 0.125f, 0.625f), 1.0f );

			if ( block_to_place == 2 ) pushToTexturedSpriteBatch( &cursor_sb, glm::vec3(gPos.x, gPos.y, -(mouse_grid_x-world_cutoff_height+1 + mouse_grid_z-world_cutoff_height+1) + (world_cutoff_height-1)*2 + 0.5f ), glm::vec2(1), 0, glm::vec2(32, 32), glm::vec2(0.5f, 1.0f), glm::vec4(0.125, 0.500f, 0.250f, 0.625f), 1.0f );
			if ( block_to_place == 3 ) pushToTexturedSpriteBatch( &cursor_sb, glm::vec3(gPos.x, gPos.y, -(mouse_grid_x-world_cutoff_height+1 + mouse_grid_z-world_cutoff_height+1) + (world_cutoff_height-1)*2 + 0.5f ), glm::vec2(1), 0, glm::vec2(32, 32), glm::vec2(0.5f, 1.0f), glm::vec4(0.250, 0.500f, 0.375f, 0.625f), 1.0f );
			if ( block_to_place == 4 ) pushToTexturedSpriteBatch( &cursor_sb, glm::vec3(gPos.x, gPos.y, -(mouse_grid_x-world_cutoff_height+1 + mouse_grid_z-world_cutoff_height+1) + (world_cutoff_height-1)*2 + 0.5f ), glm::vec2(1), 0, glm::vec2(32, 32), glm::vec2(0.5f, 1.0f), glm::vec4(0.375, 0.500f, 0.500f, 0.625f), 1.0f );
			if ( block_to_place == 5 ) pushToTexturedSpriteBatch( &cursor_sb, glm::vec3(gPos.x, gPos.y, -(mouse_grid_x-world_cutoff_height+1 + mouse_grid_z-world_cutoff_height+1) + (world_cutoff_height-1)*2 + 0.5f ), glm::vec2(1), 0, glm::vec2(32, 32), glm::vec2(0.5f, 1.0f), glm::vec4(0.500, 0.500f, 0.625f, 0.625f), 1.0f );
		buildTexturedSpriteBatch( &cursor_sb, cursor_sb.shaderID );
	}

	static bool q_pressed = false;
	if ( down_keys['q'] ) {
		// if ( !q_pressed ) {
			world_cutoff_height--;
			if ( world_cutoff_height < 1 ) world_cutoff_height = 1;
			generate_world_mesh_layer( world_cutoff_height-1 );
		// }
		q_pressed = true;
	} else {
		q_pressed = false;
	}

	static bool e_pressed = false;
	if ( down_keys['e'] ) {
		// if ( !e_pressed ) {
			world_cutoff_height++;
			if ( world_cutoff_height > World::SIZE_Y-1 ) world_cutoff_height = World::SIZE_Y-1;
			generate_world_mesh_layer( world_cutoff_height-1 );
			generate_world_mesh_layer( world_cutoff_height-2, true );
		// }
		e_pressed = true;
	} else {
		e_pressed = false;
	}

	static bool i_pressed = false;
	if ( down_keys['i'] ) {
		if ( !i_pressed ) {
			world_cutoff_height--;
			if ( world_cutoff_height < 1 ) world_cutoff_height = 1;
			generate_world_mesh_layer( world_cutoff_height-1 );
			game_cameraPosition += glm::vec3(0, 16, 0);
			game_viewMatrix = glm::translate( glm::scale(glm::mat4(1), glm::vec3(1.0f/game_camera_scale, 1.0f/game_camera_scale, 1)), -game_cameraPosition );
		}
		i_pressed = true;
	} else {
		i_pressed = false;
	}

	static bool p_pressed = false;
	if ( down_keys['p'] ) {
		if ( !p_pressed ) {
			world_cutoff_height++;
			if ( world_cutoff_height > World::SIZE_Y-1 ) world_cutoff_height = World::SIZE_Y-1;
			generate_world_mesh_layer( world_cutoff_height-1 );
			generate_world_mesh_layer( world_cutoff_height-2, true );
			game_cameraPosition += glm::vec3(0, -16, 0);
			game_viewMatrix = glm::translate( glm::scale(glm::mat4(1), glm::vec3(1.0f/game_camera_scale, 1.0f/game_camera_scale, 1)), -game_cameraPosition ); 
		}
		p_pressed = true;
	} else {
		p_pressed = false;
	}

	static bool z_pressed = false;
	if ( down_keys['z'] ) {
		if ( !z_pressed ) {
			render_half_height = !render_half_height;
		}
		z_pressed = true;
	} else {
		z_pressed = false;
	}

	main_menu.input();
}

void update_game() {
	static mach_timebase_info_data_t info;
	if ( mach_timebase_info (&info) != KERN_SUCCESS ) { printf ("mach_timebase_info failed\n"); }
	static uint64_t startTime = 0;
	uint64_t endTime = mach_absolute_time();
	uint64_t nanosecs = (endTime - startTime) * info.numer / info.denom;
	startTime = mach_absolute_time();

	delta_time = 1.0f/1000.0f*(float)nanosecs/1000000;


	main_menu.update();

	glm::vec2 aspect = glm::vec2( (float)render_dimensions.x/render_dimensions.y*10, (float)render_dimensions.x/render_dimensions.y*render_dimensions.y/render_dimensions.x*10 );
	float gmp_x = 1.0f/render_dimensions.x*mouse_position.x*aspect.x*game_camera_scale-aspect.x*game_camera_scale/2+game_cameraPosition.x;
	float gmp_y = 1.0f/render_dimensions.y*mouse_position.y*aspect.y*game_camera_scale-aspect.y*game_camera_scale/2+game_cameraPosition.y; //+ 16.0f;
	glm::vec2 x_vector = glm::vec2(0.5f, -0.25f);
	glm::vec2 z_vector = glm::vec2(-0.5f, -0.25f);
	float mouse_grid_z = -ceil((gmp_y / 16.0f) + (gmp_x / 32.0f));
	float mouse_grid_x = -ceil((-gmp_x / 32.0f) + (gmp_y / 16.0f));
	glm::vec2 gPos = mouse_grid_x*x_vector*32.0f + mouse_grid_z*z_vector*32.0f; //* (x_vector*32.0f) * (z_vector*32.0f) * (y_vector*16.0f);

	create_text_mesh( 
		(
			"dt: " + std::to_string(delta_time*1000.0f) + 
			"\nrd: " + std::to_string((int)render_dimensions.x) + "x" + std::to_string((int)render_dimensions.y) +
			"\nwd: " + std::to_string((int)window_size.x) + "x" + std::to_string((int)window_size.y) +
			"\nvd: " + std::to_string((int)gl_viewport_size.x) + "x" + std::to_string((int)gl_viewport_size.y) +
			"\n" + std::to_string(gPos.x) + ", " + std::to_string(gPos.y) +
			"\n" + std::to_string(mouse_grid_z) + ", " + std::to_string(mouse_grid_x) + 
			"\nch: " + std::to_string(world_cutoff_height) +
			"\nScroll: " + std::to_string(game_camera_scale)
		).c_str(), debug_text_mesh, debug_pgt, debug_text_shader_id );
}

void render_game() {

	
	
	static unsigned int fbo = 0;
	static unsigned int renderedTexture = 0;
	static unsigned int depthTexture = 0;
	
	if ( dynamic_resolution ) { 
		glViewport(0, 0, gl_viewport_size.x, gl_viewport_size.y); 
		// glViewport(0, 0, gl_viewport_size.x, gl_viewport_size.y);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		glBindTexture(GL_TEXTURE_2D, 0);
	}
	else { 
		glViewport(0, 0, render_dimensions.x, render_dimensions.y); 
		if ( fbo == 0 ) glGenFramebuffers(1, &fbo);
		
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		
		if ( renderedTexture == 0 ) glGenTextures(1, &renderedTexture);
		glBindTexture(GL_TEXTURE_2D, renderedTexture);
		// Give an empty image to OpenGL ( the last "0" )
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, render_dimensions.x, render_dimensions.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, renderedTexture, 0);

		if ( depthTexture == 0 ) glGenTextures(1, &depthTexture);
		glBindTexture(GL_TEXTURE_2D, depthTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, render_dimensions.x, render_dimensions.y, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL );
		// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, depthTexture, 0);
	}







	glClearColor(25/255.0f, 25/255.0f, 25/255.0f, 1);
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	glUseProgram( world.shaderID );

		setUniformMat4( world.shaderID, "view", game_viewMatrix );
		setUniformMat4( world.shaderID, "projection", game_projectionMatrix );
		
		// for (int y = 0; y < world_cutoff_height; ++y) {
		// 	setUniform4f( world.shaderID, "tintColor", glm::vec4( glm::vec3( 0.5f + 0.5f*(1.0f/World::SIZE_Y*(y+1)+1.0f/World::SIZE_Y*(World::SIZE_Y-world_cutoff_height))  ), 1.0f) );
		// 	unsigned int used_texture = world.texID;
		// 	if ( render_half_height && y == world_cutoff_height-1 ) used_texture = half_height_texture;
		// 	renderTexturedSpriteBatch( &world.tile_sb[y], world.shaderID, used_texture );
		// }

		for (int y = 0; y < world_cutoff_height; ++y) {
			
			float tint_value = 0.7f;
			if ( y+20 > world_cutoff_height ) tint_value = 0.7f + 0.3f/20*(20 - (world_cutoff_height - y));

			setUniform4f( world.shaderID, "tintColor", glm::vec4( glm::vec3( tint_value  ), 1.0f) );
			

			unsigned int used_texture = world.texID;
			if ( render_half_height && y == world_cutoff_height-1 ) used_texture = half_height_texture;
			renderTexturedSpriteBatch( &world.tile_sb[y], world.shaderID, used_texture );
		}

	if ( !cursor_disable_depth ) glClear( GL_DEPTH_BUFFER_BIT );
	glUseProgram( cursor_sb.shaderID );

		setUniformMat4( cursor_sb.shaderID, "view", game_viewMatrix );
		setUniformMat4( cursor_sb.shaderID, "projection", game_projectionMatrix );
		setUniform4f( cursor_sb.shaderID, "tintColor", glm::vec4(1.0f) );

		unsigned int used_texture = cursor_sb.texID;
		if ( render_half_height ) used_texture = half_height_texture;
		renderTexturedSpriteBatch( &cursor_sb, cursor_sb.shaderID, used_texture );

	glClear( GL_DEPTH_BUFFER_BIT );
	main_menu.render();

	glClear( GL_DEPTH_BUFFER_BIT );
	glUseProgram( debug_text_shader_id );
		
		setUniformMat4( debug_text_shader_id, "view", viewMatrix );
		setUniformMat4( debug_text_shader_id, "projection", projectionMatrix );
		setUniform4f( debug_text_shader_id, "overlayColor", glm::vec4(1.0f) );
		render_text_mesh( debug_text_mesh, debug_text_shader_id );

	glUseProgram( 0 );





	if ( !dynamic_resolution ) {
		// glFlush();

		// glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);
		// glBlitFramebuffer(0, 0, gl_viewport_size.x, gl_viewport_size.y, 0, 0, 640, 480, GL_COLOR_BUFFER_BIT, GL_NEAREST);
		// glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		float scale_factor = 1;
		if ( gl_viewport_size.x / gl_viewport_size.y >= render_dimensions.x / render_dimensions.y ) scale_factor = gl_viewport_size.y / render_dimensions.y;
		else scale_factor = gl_viewport_size.x / render_dimensions.x;

		// glViewport(0, 0, 640, 480);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		glBlitFramebuffer(0, 0, render_dimensions.x, render_dimensions.y, (gl_viewport_size.x-render_dimensions.x*scale_factor)/2, (gl_viewport_size.y-render_dimensions.y*scale_factor)/2, render_dimensions.x*scale_factor + (gl_viewport_size.x-render_dimensions.x*scale_factor)/2, render_dimensions.y*scale_factor + (gl_viewport_size.y-render_dimensions.y*scale_factor)/2, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST);
	}
}



////////////////////////////////////
/////////////////////////
//////////////////////////////



void set_mouse_position( float x, float y ) {
	mouse_position = glm::vec2( x, window_size.y-y );
	if ( !dynamic_resolution ) {
		float scale_factor = 1;
		if ( window_size.x / window_size.y >= render_dimensions.x / render_dimensions.y ) scale_factor = window_size.y / render_dimensions.y;
		else scale_factor = window_size.x / render_dimensions.x;

		mouse_position.x =  mouse_position.x/scale_factor - (window_size.x-render_dimensions.x*scale_factor)/2 / scale_factor;
		mouse_position.y =  mouse_position.y/scale_factor - (window_size.y-render_dimensions.y*scale_factor)/2 / scale_factor;
	}
}

void set_mouse_state( unsigned int state ) {
	mouse_state = state;
}

void set_mouse_scroll_value( float scroll_value ) {
	mouse_scroll = scroll_value;
}

// void move_game_camera( float x, float y ) {
// 	game_cameraPosition += glm::vec3(x, y, 0)*delta_time*300.0f;
// 	game_viewMatrix = glm::translate( glm::scale(glm::mat4(1), glm::vec3(1.0f/game_camera_scale, 1.0f/game_camera_scale, 1)), -game_cameraPosition ); 
// }

void resize_view( float ww, float wh, float glvw, float glvh ) {
	window_size = glm::vec2( ww, wh);
	gl_viewport_size = glm::vec2( glvw, glvh );
	if ( dynamic_resolution ) { 
		render_dimensions = window_size; 
	}

	projectionMatrix = glm::ortho( 0.0f, render_dimensions.x, render_dimensions.y, 0.0f, 0.1f, 1000.0f);
	glm::vec2 aspect = glm::vec2( (float)render_dimensions.x/render_dimensions.y*10, (float)render_dimensions.x/render_dimensions.y*render_dimensions.y/render_dimensions.x*10 );
	game_projectionMatrix = glm::ortho( -aspect.x/2, aspect.x/2, aspect.y/2, -aspect.y/2, 0.1f, 2000.0f);
}



//////////////////////////////////////////
///////////////////////////////////
///////////////////////////

