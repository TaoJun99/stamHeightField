#version 330 core

layout (location = 0) in vec3 aPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform float size;
uniform sampler2D inputTexture;

out vec2 texCoords;

void main() {
    texCoords = (aPos.xz + size / 2) / size;

    float height = texture(inputTexture, texCoords).x;
    vec3 position = vec3(aPos.x, height, aPos.z);

    gl_Position = projection * view * model * vec4(position, 1.0);
}