#version 330 core

uniform sampler2D inputTexture;

in vec2 texCoords;

out vec4 fragColor;

void main() {
//    fragColor = vec4(0.0, 0.5, 0.5, 1.0);
    fragColor = texture(inputTexture, texCoords);
}