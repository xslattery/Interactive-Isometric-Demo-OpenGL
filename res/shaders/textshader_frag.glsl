#version 330

in vec2 TexCoord;
in vec4 inColor;

uniform sampler2D ourTexture;
uniform vec4 overlayColor;

out vec4 Color;

void main() {
    if ( texture(ourTexture, TexCoord).r == 0 ) discard;
    Color = vec4( inColor.xyz*overlayColor.xyz, texture(ourTexture, TexCoord).r );
}
