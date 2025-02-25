#version 330 core

uniform sampler2D w; //vector field
uniform float halfrdx;
uniform int gridSize;

in vec2 texCoords;
out vec4 fragColor;

bool isBoundary(ivec2 gridCellIndex) {
    return gridCellIndex.x == 0 || gridCellIndex.x == gridSize - 1 ||
    gridCellIndex.y == 0 || gridCellIndex.y == gridSize - 1;
}

void main() {
    ivec2 gridCellIndex = ivec2(floor(texCoords * gridSize));  // Convert normalized to integer coordinates

    vec4 wC = texelFetch(w, gridCellIndex, 0);

    // Fetch neighboring texels
//    vec4 wL = texelFetch(w, gridCellIndex - ivec2(1, 0), 0);  // Left
//    vec4 wR = texelFetch(w, gridCellIndex + ivec2(1, 0), 0);  // Right
//    vec4 wB = texelFetch(w, gridCellIndex - ivec2(0, 1), 0);  // Bottom
//    vec4 wT = texelFetch(w, gridCellIndex + ivec2(0, 1), 0);  // Top


    vec4 wL = texture(w, texCoords + vec2(-1.0 / gridSize, 0));
    vec4 wR = texture(w, texCoords + vec2( 1.0 / gridSize, 0));
    vec4 wB = texture(w, texCoords + vec2(0, -1.0 / gridSize));
    vec4 wT = texture(w, texCoords + vec2(0,  1.0 / gridSize));

    fragColor = vec4(halfrdx * ((wR.x - wL.x) + (wT.y - wB.y)), 0.0, 0.0, 0.0);
}