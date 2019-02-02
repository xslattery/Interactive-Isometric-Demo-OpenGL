
#ifndef _sprite_hpp_
#define _sprite_hpp_

void LoadTexture( unsigned int* tex_id, const char* name );

struct Nine_Sliced_Sprite {
	glm::mat4 transform_matrix;

	unsigned int vao = 0;
	unsigned int vbo = 0;
	unsigned int vbo_tex = 0;
	unsigned int ebo = 0;
	unsigned char num_indices = 0;
};

void create_nine_sliced_sprite( Nine_Sliced_Sprite& nss, glm::vec2 size, glm::vec2 csz, glm::vec2 pivot, unsigned int shader );
void render_nine_sliced_sprite( Nine_Sliced_Sprite& nss, unsigned int shader, unsigned int texture );


struct ColoredSpriteBatch {
	
	unsigned int vao = 0;
	unsigned int vbo = 0;
	unsigned int vbo_color = 0;
	unsigned int ebo = 0;
	unsigned int shaderID = 0;

	std::vector<GLfloat> vertices;
	std::vector<unsigned char> vertex_colors;
	std::vector<unsigned int> indices;
	unsigned int numIndices = 0;

	~ColoredSpriteBatch() {
		if ( vao != 0 ) glDeleteVertexArrays( 1, &vao );
		if ( vbo != 0 ) glDeleteBuffers( 1, &vbo );
		if ( vbo_color != 0 ) glDeleteBuffers( 1, &vbo_color );
		if ( ebo != 0 ) glDeleteBuffers( 1, &ebo );
	}
};

void prepairColoredSpriteBatchForPush( ColoredSpriteBatch* sb ); // This will clear the batch if it is already made.
void pushToColoredSpriteBatch( ColoredSpriteBatch* sb, glm::vec3 pos, glm::vec2 scale, float rot, glm::vec2 size, glm::vec2 pvt, glm::vec4 color ); // This will add a sprite to the batch.
void buildColoredSpriteBatch( ColoredSpriteBatch* sb ); // This will send off all the data to opengl. And clear the data from the vertex and index vectors.
void renderColoredSpriteBatch( ColoredSpriteBatch* sb ); // This will render the sprite batch to the screen.


struct TexturedSpriteBatch {
	
	unsigned int vao = 0;
	unsigned int vbo = 0;
	unsigned int vbo_tex = 0;
	unsigned int vbo_color = 0;
	unsigned int ebo = 0;
	unsigned int shaderID = 0;
	unsigned int texID = 0;

	std::vector<GLfloat> vertices;
	std::vector<float> vertex_tex;
	std::vector<unsigned char> vertex_colors;
	std::vector<unsigned int> indices;
	unsigned int numIndices = 0;

	~TexturedSpriteBatch() {
		if ( vao != 0 ) glDeleteVertexArrays( 1, &vao );
		if ( vbo != 0 ) glDeleteBuffers( 1, &vbo );
		if ( vbo_tex != 0 ) glDeleteBuffers( 1, &vbo_tex );
		if ( vbo_color != 0 ) glDeleteBuffers( 1, &vbo_color );
		if ( ebo != 0 ) glDeleteBuffers( 1, &ebo );
	}
};

void prepairTexturedSpriteBatchForPush( TexturedSpriteBatch* sb ); // This will clear the batch if it is already made.
void pushToTexturedSpriteBatch( TexturedSpriteBatch* sb, glm::vec3 pos, glm::vec2 scale, float rot, glm::vec2 size, glm::vec2 pvt, glm::vec4 texcoord, float tint ); // This will add a sprite to the batch.
void buildTexturedSpriteBatch( TexturedSpriteBatch* sb, unsigned int shaderID ); // This will send off all the data to opengl. And clear the data from the vertex and index vectors.
void renderTexturedSpriteBatch( TexturedSpriteBatch* sb, unsigned int shaderID, unsigned int texID ); // This will render the sprite batch to the screen.


#endif