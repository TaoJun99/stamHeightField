#version 330 core

uniform sampler2D p; // pressure field
uniform sampler2D w; // velocity
uniform float halfrdx;
uniform int gridSize;

in vec2 texCoords;
out vec4 fragColor;

bool isBoundary(ivec2 gridCellIndex) {
    return gridCellIndex.x == 0 || gridCellIndex.x == gridSize - 1 ||
    gridCellIndex.x == 0 || gridCellIndex.x == gridSize - 1;
}

void main() {
    ivec2 gridCellIndex = ivec2(floor(texCoords * gridSize));

    if (gridCellIndex.x == 0) {
        if (gridCellIndex.y == 0 || gridCellIndex.y == gridSize - 1) { // Corner
            fragColor = vec4(0.0, 0.0, 0.0, 0.0);
        } else {
            fragColor = -texelFetch(w,gridCellIndex + ivec2(1, 0), 0);
        }
    } else if (gridCellIndex.x == gridSize - 1) {
        if (gridCellIndex.y == 0 || gridCellIndex.y == gridSize - 1) { // Corner
            fragColor = vec4(0.0, 0.0, 0.0, 0.0);
        } else {
            fragColor = -texelFetch(w,gridCellIndex - ivec2(1, 0), 0);
        }
    } else if (gridCellIndex.y == 0) {
        fragColor = -texelFetch(w,gridCellIndex + ivec2(0, 1), 0);
    } else if (gridCellIndex.y == gridSize - 1) {
        fragColor = -texelFetch(w, gridCellIndex - ivec2(0, 1), 0);
    } else {
        float pC = texelFetch(p, gridCellIndex, 0).x;

        float pL = texelFetch(p, gridCellIndex - ivec2(1, 0), 0).x;  // Left
        float pR = texelFetch(p, gridCellIndex + ivec2(1, 0), 0).x;  // Right
        float pB = texelFetch(p, gridCellIndex - ivec2(0, 1), 0).x;  // Bottom
        float pT = texelFetch(p, gridCellIndex + ivec2(0, 1), 0).x;  // Top

        if (isBoundary(gridCellIndex - ivec2(1, 0))) {
            pL = pC;
        }
        if (isBoundary(gridCellIndex + ivec2(1, 0))) {
            pR = pC;
        }
        if (isBoundary(gridCellIndex - ivec2(0, 1))) {
            pB = pC;
        }
        if (isBoundary(gridCellIndex + ivec2(0, 1))) {
            pT = pC;
        }

        fragColor = texelFetch(w, gridCellIndex, 0);
        fragColor.xy = fragColor.xy - halfrdx * vec2(pR - pL, pT - pB);
    }
}