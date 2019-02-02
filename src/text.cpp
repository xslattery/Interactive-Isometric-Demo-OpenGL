//
//  text.cpp
//  2DProject
//
//  Created by Xavier Slattery.
//  Copyright © 2015 Xavier Slattery. All rights reserved.
//

#include <OpenGL/gl3.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <map>
#include <vector>

#include "debug.hpp"
#include "shader.hpp"
#include "text.hpp"


void create_packed_glyph_texture( Packed_Glyph_Texture &pgt, const char* filename, FT_Library freeType, unsigned int filter ) {

	if ( pgt.fontsize > 200 ) pgt.fontsize = 200; // NOTE: The max size will be 200pixels aka 100pt
	
	FT_Face face;
	if ( FT_New_Face( freeType, filename, 0, &face ) ) { ERROR( "FREETYPE: Failed to load font.\n" ); return; }	
	FT_Set_Pixel_Sizes( face, 0, pgt.fontsize );
	if ( FT_Load_Char( face, 'X', FT_LOAD_RENDER ) ) { ERROR( "FREETYTPE: Failed to load Glyph\n" ); return; }
	
	struct Temp_Character {
		glm::ivec2 size; 
		glm::ivec2 bearing;	
		unsigned int advance;	
		unsigned char* bitmap;
		~Temp_Character() { delete [] bitmap; }
	};
	std::map< unsigned char, Temp_Character > temp_glyphs;

	unsigned char startCharacter = 0;
	unsigned char endCharacter = 128;

	for ( unsigned char c = startCharacter; c <= endCharacter; c++ ) {
		if ( FT_Load_Char( face, c, FT_LOAD_RENDER ) ) {
			ERROR( "ERROR::FREETYTPE: Failed to load Glyph" );
			continue;
		}

		temp_glyphs.insert( std::pair<unsigned char, Temp_Character>( c, Temp_Character() ) );
		temp_glyphs[c].size = glm::ivec2( face->glyph->bitmap.width, face->glyph->bitmap.rows );
		temp_glyphs[c].bearing = glm::ivec2( face->glyph->bitmap_left, face->glyph->bitmap_top );
		temp_glyphs[c].advance = (unsigned int)face->glyph->advance.x;
		unsigned int dimensions = (unsigned int)face->glyph->bitmap.width * (unsigned int)face->glyph->bitmap.rows;
		temp_glyphs[c].bitmap = new unsigned char [ dimensions ];
		
		memcpy( temp_glyphs[c].bitmap, face->glyph->bitmap.buffer, dimensions );

		pgt.glyphs[c].size = glm::ivec2( face->glyph->bitmap.width, face->glyph->bitmap.rows );
		pgt.glyphs[c].bearing = glm::ivec2( face->glyph->bitmap_left, face->glyph->bitmap_top );
		pgt.glyphs[c].advance = (unsigned int)face->glyph->advance.x;
	}

	FT_Done_Face( face );

	unsigned int combined_character_area = 0;
	for ( size_t i = startCharacter; i <= endCharacter; i++ ) { 
		combined_character_area += temp_glyphs[i].size.x * temp_glyphs[i].size.y; 
	}

	unsigned int recm_dim = sqrt( combined_character_area ) * 1.5;
	recm_dim = recm_dim + (4 - (recm_dim%4));
	pgt.width = recm_dim;
	pgt.height = recm_dim;
	unsigned char* combinedBitmap = new unsigned char [ recm_dim * recm_dim ]();
	
	unsigned int xx = 0; unsigned int yy = 0;
	for ( size_t ch = startCharacter; ch <= endCharacter; ch++ ) {
		if ( temp_glyphs[ch].size.x > 0 ) { 
			if ( xx + temp_glyphs[ch].size.x + 1 > recm_dim ) { yy += pgt.fontsize; xx = 0; }
			if ( yy + temp_glyphs[ch].size.y + 1 > recm_dim ) { break; }
			pgt.glyphs[ch].position = glm::vec2( xx, yy );
			for ( size_t y = yy; y < yy+temp_glyphs[ch].size.y; y++ ) {
				for ( size_t x = xx; x < xx+temp_glyphs[ch].size.x; x++ ) {
					combinedBitmap[ recm_dim*y + x ] = temp_glyphs[ch].bitmap[ temp_glyphs[ch].size.x*(y-yy) + x-xx ];
				}
			}
			xx += temp_glyphs[ch].size.x + 1;
		}
	}	    

	unsigned int tex_id;
	glGenTextures(1, &tex_id);

	glBindTexture(GL_TEXTURE_2D, tex_id);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage2D( GL_TEXTURE_2D, 0, GL_RED, recm_dim, recm_dim, 0, GL_RED, GL_UNSIGNED_BYTE, combinedBitmap );
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter); // GL_NEAREST
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter); // GL_NEAREST

	delete [] combinedBitmap;

	pgt.id = tex_id;

}

void create_text_mesh( const char* text, Text_Mesh &tm, Packed_Glyph_Texture &pgt, unsigned int shader_id ) {

	float scaleFactor = (float)pgt.fontsize / (float)tm.fontsize;

	int lineSpacing = 0;

	std::vector<float> verts;
	std::vector<unsigned char> colors;
	std::vector<unsigned int> indis;

	unsigned int vert_ofst = 0;
	float xx = 0;
	float yy = pgt.glyphs[ '`' ].bearing.y/scaleFactor; // TODO(Xavier): make it so it chooses the max bearing of all pgt.glyphs.
	for ( size_t i = 0; i < CHARACTER_COUNT; ++i ) {
		if ( yy < pgt.glyphs[i].bearing.y/scaleFactor ) {
			yy = pgt.glyphs[i].bearing.y/scaleFactor;
		}
	}

	if ( 1 ) {
		for ( size_t i = 0; i < strlen( text ); ++i ) {
			
			unsigned char ch = text[i];
			if ( ch == '\n') { xx = 0; yy += tm.fontsize + lineSpacing; continue;}
			if ( ch == '\t') { xx += 4*((int)(pgt.glyphs[' '].advance/scaleFactor) >> 6) - pgt.glyphs[' '].bearing.x/scaleFactor; continue; }
			if ( ch == ' ') { xx += ((int)(pgt.glyphs[' '].advance/scaleFactor) >> 6) - pgt.glyphs[' '].bearing.x/scaleFactor; continue; }

			float xd = pgt.glyphs[ch].size.x/scaleFactor;
			float yd = pgt.glyphs[ch].size.y/scaleFactor;
			float yo = pgt.glyphs[ch].bearing.y/scaleFactor;
			float uv_x = ( 1.0f/pgt.width )*pgt.glyphs[ ch ].position.x;
			float uv_y = ( 1.0f/pgt.height )*pgt.glyphs[ ch ].position.y;
			float uv_xd = ( 1.0f/pgt.width )*( pgt.glyphs[ ch ].position.x + pgt.glyphs[ ch ].size.x );
			float uv_yd = ( 1.0f/pgt.height )*( pgt.glyphs[ ch ].position.y + pgt.glyphs[ ch ].size.y );

			float tmp_vert_array[ 20 ] = { 
				xx, 	yy-yo, 		0,		uv_x, 		uv_y,
				xx, 	yy+yd-yo, 	0,		uv_x, 		uv_yd,
				xx+xd, 	yy+yd-yo, 	0,		uv_xd, 		uv_yd,
				xx+xd, 	yy-yo, 		0,		uv_xd, 		uv_y
			};
			verts.insert( verts.end(), tmp_vert_array, tmp_vert_array + 20 );

			// NOTE(Xavier): This is for in the future when
			// individual pgt.glyphs can have their own colors.

			unsigned char tmp_color_array[ 16 ] = { 
				255, 255, 255, 255,
				255, 255, 255, 255,
				255, 255, 255, 255,
				255, 255, 255, 255,
			};
			colors.insert( colors.end(), tmp_color_array, tmp_color_array + 16 );

			unsigned int tmp_indx_array[ 6 ] = {
				vert_ofst, vert_ofst+1, vert_ofst+2, vert_ofst, vert_ofst+2, vert_ofst+3
			};
			indis.insert( indis.end(),  tmp_indx_array, tmp_indx_array+6 );
			vert_ofst += 4;
			
			xx += ( (int)(pgt.glyphs[ ch ].advance/scaleFactor) >> 6 ) - pgt.glyphs[ ch ].bearing.x/scaleFactor;
		}
	}

	if ( verts.size() > 0 ) {
		
		tm.num_indices = indis.size();

		if ( tm.vao == 0 ) glGenVertexArrays(1, &tm.vao);
		if ( tm.vbo_vertices == 0 ) glGenBuffers(1, &tm.vbo_vertices);
		if ( tm.vbo_colors == 0 ) glGenBuffers(1, &tm.vbo_colors);
		if ( tm.ebo == 0 ) glGenBuffers(1, &tm.ebo);

		glBindVertexArray( tm.vao );
		
			glBindBuffer( GL_ARRAY_BUFFER, tm.vbo_vertices );
				glBufferData( GL_ARRAY_BUFFER, verts.size() * sizeof( float ), verts.data(), GL_DYNAMIC_DRAW );
			
				GLint posAttrib = glGetAttribLocation( shader_id, "position" );
				glVertexAttribPointer( posAttrib, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0 );
				glEnableVertexAttribArray( posAttrib );
				
				GLint texAttrib = glGetAttribLocation( shader_id, "texcoord" );
				glVertexAttribPointer( texAttrib, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)) );
				glEnableVertexAttribArray( texAttrib );
			
			glBindBuffer( GL_ARRAY_BUFFER, tm.vbo_colors );
				glBufferData( GL_ARRAY_BUFFER, colors.size() * sizeof( unsigned char ), colors.data(), GL_DYNAMIC_DRAW );
				
				GLint colorAttrib = glGetAttribLocation( shader_id, "color" );
				glVertexAttribPointer( colorAttrib, 4, GL_UNSIGNED_BYTE, GL_TRUE, 4 * sizeof(unsigned char), (void*)0 );
				glEnableVertexAttribArray( colorAttrib );

			glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, tm.ebo );
				glBufferData( GL_ELEMENT_ARRAY_BUFFER, indis.size() * sizeof( unsigned int ), indis.data(), GL_DYNAMIC_DRAW );
		
		glBindVertexArray( 0 );

		tm.texture_id = pgt.id;
	}

}

void render_text_mesh( Text_Mesh &tm, unsigned int shader_id ) {
	setUniformMat4( shader_id, "model", tm.transform );
	glBindTexture( GL_TEXTURE_2D, tm.texture_id );
	glBindVertexArray( tm.vao );
	glDrawElements( GL_TRIANGLES, tm.num_indices, GL_UNSIGNED_INT, 0 );
}