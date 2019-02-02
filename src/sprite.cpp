#include <OpenGL/gl3.h>
#include <glm/glm.hpp>
#include <glm/gtx/rotate_vector.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <vector>

#include "debug.hpp"
#include "sprite.hpp"
#include "shader.hpp"

void LoadTexture( unsigned int* tex_id, const char* name ) {
	int texWidth, texHeight, n;
	unsigned char* bitmap = stbi_load( name, &texWidth, &texHeight, &n, 4 );

	if ( bitmap ) {
		if( *tex_id == 0 ) { glGenTextures(1, tex_id); }

		glBindTexture(GL_TEXTURE_2D, *tex_id);
		glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, texWidth, texHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, bitmap );

		// glGenerateMipmap(GL_TEXTURE_2D);
		
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		stbi_image_free( bitmap );
	} else {
		ERROR( "Failed to load image.\n" );
	}
}

void create_nine_sliced_sprite( Nine_Sliced_Sprite& nss, glm::vec2 size, glm::vec2 csz, glm::vec2 pivot, unsigned int shader ) {
	GLfloat positions [108] = {
		// Top Left:
		-pivot.x*size.x,					-pivot.y*size.y,						0,
		-pivot.x*size.x + csz.x,			-pivot.y*size.y,						0,
		-pivot.x*size.x,					-pivot.y*size.y + csz.y,				0,
		-pivot.x*size.x + csz.x,			-pivot.y*size.y + csz.y,				0,

		// Top Right:
		(1.0f-pivot.x)*size.x - csz.x,		-pivot.y*size.y,						0,
		(1.0f-pivot.x)*size.x,				-pivot.y*size.y,						0,
		(1.0f-pivot.x)*size.x - csz.x,		-pivot.y*size.y + csz.y,				0,
		(1.0f-pivot.x)*size.x,				-pivot.y*size.y + csz.y,				0,

		// Bottom Left:
		-pivot.x*size.x,					(1.0f-pivot.y)*size.y - csz.y,			0,
		-pivot.x*size.x + csz.x,			(1.0f-pivot.y)*size.y - csz.y,			0,
		-pivot.x*size.x,					(1.0f-pivot.y)*size.y,					0,
		-pivot.x*size.x + csz.x,			(1.0f-pivot.y)*size.y,					0,

		// Bottom Right:
		(1.0f-pivot.x)*size.x - csz.x,		(1.0f-pivot.y)*size.y - csz.y,			0,
		(1.0f-pivot.x)*size.x,				(1.0f-pivot.y)*size.y - csz.y,			0,
		(1.0f-pivot.x)*size.x - csz.x,		(1.0f-pivot.y)*size.y,					0,
		(1.0f-pivot.x)*size.x,				(1.0f-pivot.y)*size.y,					0,

		// Centre:
		-pivot.x*size.x + csz.x,			-pivot.y*size.y + csz.y,				0,
		(1.0f-pivot.x)*size.x - csz.x,		-pivot.y*size.y + csz.y,				0,
		-pivot.x*size.x + csz.x,			(1.0f-pivot.y)*size.y - csz.y,			0,
		(1.0f-pivot.x)*size.x - csz.x,		(1.0f-pivot.y)*size.y - csz.y,			0,

		// Top Middle:
		-pivot.x*size.x + csz.x,			-pivot.y*size.y,						0,
		(1.0f-pivot.x)*size.x - csz.x,		-pivot.y*size.y,						0,
		-pivot.x*size.x + csz.x,			-pivot.y*size.y + csz.y,				0,
		(1.0f-pivot.x)*size.x - csz.x,		-pivot.y*size.y + csz.y,				0,

		// Bottom Middle:
		-pivot.x*size.x + csz.x,			(1.0f-pivot.y)*size.y - csz.y,			0,
		(1.0f-pivot.x)*size.x - csz.x,		(1.0f-pivot.y)*size.y - csz.y,			0,
		-pivot.x*size.x + csz.x,			(1.0f-pivot.y)*size.y,					0,
		(1.0f-pivot.x)*size.x - csz.x,		(1.0f-pivot.y)*size.y,					0,

		// Middle Left:
		-pivot.x*size.x,					-pivot.y*size.y + csz.y,				0,
		-pivot.x*size.x + csz.x,			-pivot.y*size.y + csz.y,				0,
		-pivot.x*size.x,					(1.0f-pivot.y)*size.y - csz.y,			0,
		-pivot.x*size.x + csz.x,			(1.0f-pivot.y)*size.y - csz.y,			0,

		// Middle Right:
		(1.0f-pivot.x)*size.x - csz.x,		-pivot.y*size.y + csz.y,				0,
		(1.0f-pivot.x)*size.x,				-pivot.y*size.y + csz.y,				0,
		(1.0f-pivot.x)*size.x - csz.x,		(1.0f-pivot.y)*size.y - csz.y,			0,
		(1.0f-pivot.x)*size.x,				(1.0f-pivot.y)*size.y - csz.y,			0,
	};

	GLfloat texcoords [72] = {
		0.0f, 0.0f,
		0.3333f, 0.0f,
		0.0f, 0.3333f,
		0.3333f, 0.3333f,

		0.6666f, 0.0f,
		1.0f, 0.0f,
		0.6666f, 0.3333f,
		1.0f, 0.3333f,

		0.0f, 0.6666f,
		0.3333f, 0.6666f,
		0.0f, 1.0f,
		0.3333f, 1.0f,

		0.6666f, 0.6666f,
		1.0f, 0.6666f,
		0.6666f, 1.0f,
		1.0f, 1.0f,

		0.3333f, 0.3333f,
		0.6666f, 0.3333f,
		0.3333f, 0.6666f,
		0.6666f, 0.6666f,

		0.3333f, 0.0f,
		0.6666f, 0.0f,
		0.3333f, 0.3333f,
		0.6666f, 0.3333f,

		0.3333f, 0.6666f,
		0.6666f, 0.6666f,
		0.3333f, 1.0f,
		0.6666f, 1.0f,

		0.0f, 0.3333f,
		0.3333f, 0.3333f,
		0.0f, 0.6666f,
		0.3333f, 0.6666f,

		0.6666f, 0.3333f,
		1.0f, 0.3333f,
		0.6666f, 0.6666f,
		1.0f, 0.6666f,
	};

	nss.num_indices = 54;
	unsigned char indices [54] = {
		0, 2, 1, 
		1, 2, 3,

		4, 6, 5, 
		5, 6, 7,

		8, 10, 9, 
		9, 10, 11,

		12, 14, 13, 
		13, 14, 15,

		16, 18, 17, 
		17, 18, 19,

		20, 22, 21, 
		21, 22, 23,

		24, 26, 25, 
		25, 26, 27,

		28, 30, 29, 
		29, 30, 31,

		32, 34, 33, 
		33, 34, 35,
	};

	if ( nss.vao == 0 ) 	glGenVertexArrays( 1, &nss.vao );
	if ( nss.vbo == 0 ) 	glGenBuffers( 1, &nss.vbo );
	if ( nss.vbo_tex == 0 ) glGenBuffers( 1, &nss.vbo_tex );
	if ( nss.ebo == 0 ) 	glGenBuffers( 1, &nss.ebo );

	glBindVertexArray( nss.vao );

		glBindBuffer( GL_ARRAY_BUFFER, nss.vbo );
			glBufferData( GL_ARRAY_BUFFER, sizeof(positions) * sizeof(GLfloat), positions, GL_STATIC_DRAW );

			unsigned int posAttrib = glGetAttribLocation( shader, "position" );
			glVertexAttribPointer( posAttrib, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (void*)0 );
			glEnableVertexAttribArray( posAttrib );

		glBindBuffer( GL_ARRAY_BUFFER, nss.vbo_tex );
			glBufferData( GL_ARRAY_BUFFER, sizeof(texcoords) * sizeof(GLfloat), texcoords, GL_STATIC_DRAW );

			unsigned int texAttrib = glGetAttribLocation( shader, "texcoord" );
			glVertexAttribPointer( texAttrib, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (void*)0 );
			glEnableVertexAttribArray( texAttrib );

		glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, nss.ebo );
			glBufferData( GL_ELEMENT_ARRAY_BUFFER, sizeof(indices) * sizeof(unsigned char), indices, GL_STATIC_DRAW );

	glBindVertexArray( 0 );
}

void render_nine_sliced_sprite( Nine_Sliced_Sprite& nss, unsigned int shader, unsigned int texture ) {
	setUniformMat4( shader, "model", nss.transform_matrix );
	glBindTexture( GL_TEXTURE_2D, texture );
	glBindVertexArray( nss.vao );
	glDrawElements( GL_TRIANGLES, nss.num_indices, GL_UNSIGNED_BYTE, 0 );
}


///////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////


void prepairColoredSpriteBatchForPush( ColoredSpriteBatch* sb ) {
	sb->vertices.clear();
	sb->vertex_colors.clear();
	sb->indices.clear();
	sb->numIndices = 0;
}


void pushToColoredSpriteBatch( ColoredSpriteBatch* sb, glm::vec3 pos, glm::vec2 scale, float rot, glm::vec2 size, glm::vec2 pvt, glm::vec4 color ) {

	unsigned int tmp_v = (unsigned int)(sb->vertices.size()/3);
	unsigned int tmp_indices [6] = {
		tmp_v+0, tmp_v+2, tmp_v+1, 
		tmp_v+1, tmp_v+2, tmp_v+3
	};
	sb->numIndices += 6;
	sb->indices.insert( sb->indices.end(), tmp_indices, tmp_indices + 6 );

	GLfloat x1 = -pvt.x * size.x * scale.x;
	GLfloat y1 = -pvt.y * size.y * scale.y;

	GLfloat x2 = (1.0f-pvt.x) * size.x * scale.x;
	GLfloat y2 = (1.0f-pvt.y) * size.y * scale.y;

	glm::vec2 xy1 = glm::rotate( glm::vec2(x1, y1), rot );
	glm::vec2 xy2 = glm::rotate( glm::vec2(x2, y1), rot );
	glm::vec2 xy3 = glm::rotate( glm::vec2(x1, y2), rot );
	glm::vec2 xy4 = glm::rotate( glm::vec2(x2, y2), rot );

	GLfloat tmp_vert_array[ 12 ] = { 
		xy1.x + pos.x,  	xy1.y + pos.y, 		pos.z,
		xy2.x + pos.x,  	xy2.y + pos.y, 		pos.z,
		xy3.x + pos.x, 		xy3.y + pos.y, 		pos.z,
		xy4.x + pos.x, 		xy4.y + pos.y, 		pos.z,
	};
	sb->vertices.insert( sb->vertices.end(), tmp_vert_array, tmp_vert_array + 12 );

	unsigned char tmp_color_array[ 16 ] = { 
		255*color.x, 255*color.y, 255*color.z, 255*color.w,
		255*color.x, 255*color.y, 255*color.z, 255*color.w,
		255*color.x, 255*color.y, 255*color.z, 255*color.w,
		255*color.x, 255*color.y, 255*color.z, 255*color.w,
	};
	sb->vertex_colors.insert( sb->vertex_colors.end(), tmp_color_array, tmp_color_array + 16 );
}

void buildColoredSpriteBatch( ColoredSpriteBatch* sb ) {
	if ( sb->vao == 0 ) glGenVertexArrays( 1, &sb->vao );
	if ( sb->vbo == 0 ) glGenBuffers( 1, &sb->vbo );
	if ( sb->vbo_color == 0 ) glGenBuffers( 1, &sb->vbo_color );
	if ( sb->ebo == 0 ) glGenBuffers( 1, &sb->ebo );

	glBindVertexArray( sb->vao );

		glBindBuffer( GL_ARRAY_BUFFER, sb->vbo );
			glBufferData( GL_ARRAY_BUFFER, sb->vertices.size() * sizeof(GLfloat), sb->vertices.data(), GL_DYNAMIC_DRAW );

			unsigned int posAttrib = glGetAttribLocation( sb->shaderID, "position" );
			glVertexAttribPointer( posAttrib, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (void*)0 );
			glEnableVertexAttribArray( posAttrib );

		glBindBuffer( GL_ARRAY_BUFFER, sb->vbo_color );
			glBufferData( GL_ARRAY_BUFFER, sb->vertex_colors.size() * sizeof(unsigned char), sb->vertex_colors.data(), GL_DYNAMIC_DRAW );

			unsigned int colorAttrib = glGetAttribLocation( sb->shaderID, "color" );
			glVertexAttribPointer( colorAttrib, 4, GL_UNSIGNED_BYTE, GL_TRUE, 4 * sizeof(unsigned char), (void*)0 );
			glEnableVertexAttribArray( colorAttrib );

		glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, sb->ebo );
			glBufferData( GL_ELEMENT_ARRAY_BUFFER, sb->indices.size() * sizeof(unsigned int), sb->indices.data(), GL_DYNAMIC_DRAW );
	
	glBindVertexArray( 0 );
}

void renderColoredSpriteBatch( ColoredSpriteBatch* sb ) {
	setUniformMat4( sb->shaderID, "model", glm::mat4(1) );
	glBindVertexArray( sb->vao );
	glDrawElements( GL_TRIANGLES, sb->numIndices, GL_UNSIGNED_INT, 0 );
}


///////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////


void prepairTexturedSpriteBatchForPush( TexturedSpriteBatch* sb ) {
	sb->vertices.clear();
	sb->vertex_tex.clear();
	sb->indices.clear();
	sb->vertex_colors.clear();
	sb->numIndices = 0;
}


void pushToTexturedSpriteBatch( TexturedSpriteBatch* sb, glm::vec3 pos, glm::vec2 scale, float rot, glm::vec2 size, glm::vec2 pvt, glm::vec4 texcoord, float tint ) {

	unsigned int tmp_v = (unsigned int)(sb->vertices.size()/3);
	unsigned int tmp_indices [6] = {
		tmp_v+0, tmp_v+2, tmp_v+1, 
		tmp_v+1, tmp_v+2, tmp_v+3
	};
	sb->numIndices += 6;
	sb->indices.insert( sb->indices.end(), tmp_indices, tmp_indices + 6 );

	GLfloat x1 = -pvt.x * size.x * scale.x;
	GLfloat y1 = -pvt.y * size.y * scale.y;

	GLfloat x2 = (1.0f-pvt.x) * size.x * scale.x;
	GLfloat y2 = (1.0f-pvt.y) * size.y * scale.y;

	glm::vec2 xy1 = glm::rotate( glm::vec2(x1, y1), rot );
	glm::vec2 xy2 = glm::rotate( glm::vec2(x2, y1), rot );
	glm::vec2 xy3 = glm::rotate( glm::vec2(x1, y2), rot );
	glm::vec2 xy4 = glm::rotate( glm::vec2(x2, y2), rot );

	GLfloat tmp_vert_array[ 12 ] = { 
		xy1.x + pos.x,  	xy1.y + pos.y, 		pos.z,
		xy2.x + pos.x,  	xy2.y + pos.y, 		pos.z,
		xy3.x + pos.x, 		xy3.y + pos.y, 		pos.z,
		xy4.x + pos.x, 		xy4.y + pos.y, 		pos.z,
	};
	sb->vertices.insert( sb->vertices.end(), tmp_vert_array, tmp_vert_array + 12 );

	float tmp_tex_array[ 8 ] = { 
		texcoord.x, texcoord.y,
		texcoord.z, texcoord.y,
		texcoord.x, texcoord.w,
		texcoord.z, texcoord.w
	};
	sb->vertex_tex.insert( sb->vertex_tex.end(), tmp_tex_array, tmp_tex_array + 8 );

	unsigned char tmp_color_array[ 16 ] = { 
		255*tint, 255*tint, 255*tint, 255,
		255*tint, 255*tint, 255*tint, 255,
		255*tint, 255*tint, 255*tint, 255,
		255*tint, 255*tint, 255*tint, 255,
	};
	sb->vertex_colors.insert( sb->vertex_colors.end(), tmp_color_array, tmp_color_array + 16 );
}

void buildTexturedSpriteBatch( TexturedSpriteBatch* sb, unsigned int shaderID ) {
	if ( sb->vao == 0 ) glGenVertexArrays( 1, &sb->vao );
	if ( sb->vbo == 0 ) glGenBuffers( 1, &sb->vbo );
	if ( sb->vbo_tex == 0 ) glGenBuffers( 1, &sb->vbo_tex );
	if ( sb->vbo_color == 0 ) glGenBuffers( 1, &sb->vbo_color );
	if ( sb->ebo == 0 ) glGenBuffers( 1, &sb->ebo );

	glBindVertexArray( sb->vao );

		glBindBuffer( GL_ARRAY_BUFFER, sb->vbo );
			glBufferData( GL_ARRAY_BUFFER, sb->vertices.size() * sizeof(GLfloat), sb->vertices.data(), GL_DYNAMIC_DRAW );

			unsigned int posAttrib = glGetAttribLocation( shaderID, "position" );
			glVertexAttribPointer( posAttrib, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (void*)0 );
			glEnableVertexAttribArray( posAttrib );

		glBindBuffer( GL_ARRAY_BUFFER, sb->vbo_tex );
			glBufferData( GL_ARRAY_BUFFER, sb->vertex_tex.size() * sizeof(float), sb->vertex_tex.data(), GL_DYNAMIC_DRAW );

			unsigned int texAttrib = glGetAttribLocation( shaderID, "texcoord" );
			glVertexAttribPointer( texAttrib, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0 );
			glEnableVertexAttribArray( texAttrib );

		glBindBuffer( GL_ARRAY_BUFFER, sb->vbo_color );
			glBufferData( GL_ARRAY_BUFFER, sb->vertex_colors.size() * sizeof(unsigned char), sb->vertex_colors.data(), GL_DYNAMIC_DRAW );

			unsigned int colorAttrib = glGetAttribLocation( shaderID, "color" );
			glVertexAttribPointer( colorAttrib, 4, GL_UNSIGNED_BYTE, GL_TRUE, 4 * sizeof(unsigned char), (void*)0 );
			glEnableVertexAttribArray( colorAttrib );

		glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, sb->ebo );
			glBufferData( GL_ELEMENT_ARRAY_BUFFER, sb->indices.size() * sizeof(unsigned int), sb->indices.data(), GL_DYNAMIC_DRAW );
	
	glBindVertexArray( 0 );
}

void renderTexturedSpriteBatch( TexturedSpriteBatch* sb, unsigned int shaderID, unsigned int texID ) {
	setUniformMat4( shaderID, "model", glm::mat4(1) );
	glBindTexture( GL_TEXTURE_2D, texID );
	glBindVertexArray( sb->vao );
	glDrawElements( GL_TRIANGLES, sb->numIndices, GL_UNSIGNED_INT, 0 );
}
