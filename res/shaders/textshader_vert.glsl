#version 330

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 texcoord;
layout(location = 2) in vec4 color;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec2 TexCoord;
out vec4 inColor;

void main () {
    gl_Position = projection * view * model * vec4(position, 1.0);
    TexCoord = texcoord;
    inColor = color;
}

