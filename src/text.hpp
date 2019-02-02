//
//  text.hpp
//  2DProject
//
//  Created by Xavier Slattery.
//  Copyright © 2015 Xavier Slattery. All rights reserved.
//

#ifndef _Text_hpp
#define _Text_hpp

struct Glyph {
	glm::vec2 position;
	glm::ivec2 size;
	glm::ivec2 bearing;
	unsigned int advance;
};

struct Packed_Glyph_Texture {
	int width;
	int height;
	int fontsize;
	unsigned int id;

	#define CHARACTER_COUNT 256
	Glyph glyphs[ CHARACTER_COUNT ];
};

struct Text_Mesh {
	int crop_width;
	int crop_height;
	int fontsize;

	glm::vec3 position;
	glm::mat4 transform;

	unsigned int texture_id;
	unsigned int vao;
	unsigned int vbo_vertices;
	unsigned int vbo_colors;
	unsigned int ebo;
	unsigned int num_indices;
};

void create_packed_glyph_texture( Packed_Glyph_Texture &pgt, const char* filename, FT_Library freeType, unsigned int filter = GL_LINEAR );
void create_text_mesh( const char* text, Text_Mesh &tm, Packed_Glyph_Texture &pgt, unsigned int shader_id );
void render_text_mesh( Text_Mesh &tm, unsigned int shader_id );

#endif