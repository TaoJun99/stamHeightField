#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 texCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform vec2 textureScale;

void main() {
//    texCoord = aTexCoord;
    texCoord = aPos.xz;
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
