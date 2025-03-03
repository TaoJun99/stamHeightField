#version 330 core

uniform sampler2D velocityTexture; //vector field
uniform sampler2D heightField;
uniform float halfrdx;
uniform int gridSize;
uniform float timeStep;

in vec2 texCoords;
out vec4 fragColor;

bool isBoundary(vec2 texCoords) {
    return texCoords.x == 0 || texCoords.x == 1 ||
    texCoords.y == 0 || texCoords.y == 1;
}

void main() {
    ivec2 gridCellIndex = ivec2(floor(texCoords * (gridSize - 1)));  // Convert normalized to integer coordinates

    vec4 v = texture(velocityTexture, texCoords);
    vec4 vL = texture(velocityTexture, texCoords + vec2(-1.0 / gridSize, 0));
    vec4 vR = texture(velocityTexture, texCoords + vec2( 1.0 / gridSize, 0));
    vec4 vB = texture(velocityTexture, texCoords + vec2(0, -1.0 / gridSize));
    vec4 vT = texture(velocityTexture, texCoords + vec2(0,  1.0 / gridSize));

    float h = texture(heightField, texCoords).x;
    float hL = texture(heightField, texCoords + vec2( -1.0 / gridSize, 0)).x;// Left
    float hR = texture(heightField, texCoords + vec2( 1.0 / gridSize, 0)).x;// Right
    float hB = texture(heightField, texCoords + vec2(0, -1.0 / gridSize)).x;// Bottom
    float hT = texture(heightField, texCoords + vec2(0,  1.0 / gridSize)).x;// Top


    float g = 9.81;

    vec2 grad_h = halfrdx * vec2(hR - hL, hT - hB);


    if (!isBoundary(texCoords)) {
        float v_x = v.x - (g * (hR - hL) * 2 * halfrdx) * 50 * timeStep;
        float v_y = v.y - (g * (hT - hB) * 2 * halfrdx) * 50 * timeStep;

        fragColor = 0.98 * vec4(v_x, v_y, 0.0, 0.0);
    } else {
        fragColor = v;
    }

}