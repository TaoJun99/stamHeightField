#version 330 core

layout (location = 0) in vec3 aPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform float size;

out vec2 texCoords;

void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0);

    texCoords = (aPos.xz + size / 2) / size;
}