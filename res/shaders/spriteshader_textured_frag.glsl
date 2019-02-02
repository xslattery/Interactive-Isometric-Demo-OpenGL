#version 330

in vec2 TexCoord;

uniform vec4 tintColor;
uniform sampler2D ourTexture;

out vec4 Color;

void main() {
	if ( texture(ourTexture, TexCoord).w == 0 ) discard;
    Color = texture(ourTexture, TexCoord) * tintColor;
}
