#version 330 core

uniform float alpha;
uniform float rBeta;
uniform sampler2D x;
uniform sampler2D b;
uniform float gridSize;
uniform float scale; // -1: velocity, 1: pressure

in vec2 texCoords;

out vec4 fragColor;

bool isBoundary(ivec2 gridCellIndex) {
   return gridCellIndex.x == 0 || gridCellIndex.x == gridSize - 1 ||
   gridCellIndex.x == 0 || gridCellIndex.x == gridSize - 1;
}


void main() {
    // 1 Jacobi update iteration
    ivec2 gridCellIndex = ivec2(floor(texCoords * gridSize));  // Convert normalized to integer coordinates

    if (gridCellIndex.x == 0) {
        if (gridCellIndex.y == 0 || gridCellIndex.y == gridSize - 1) { // Corner
            fragColor = vec4(0.0, 0.0, 0.0, 0.0);
        } else {
            fragColor = scale * texelFetch(x,gridCellIndex + ivec2(1, 0), 0);
        }
    } else if (gridCellIndex.x == gridSize - 1) {
        if (gridCellIndex.y == 0 || gridCellIndex.y == gridSize - 1) { // Corner
            fragColor = vec4(0.0, 0.0, 0.0, 0.0);
        } else {
            fragColor = scale * texelFetch(x,gridCellIndex - ivec2(1, 0), 0);
        }
    } else if (gridCellIndex.y == 0) {
        fragColor = scale * texelFetch(x,gridCellIndex + ivec2(0, 1), 0);
    } else if (gridCellIndex.y == gridSize - 1) {
        fragColor = scale * texelFetch(x, gridCellIndex - ivec2(0, 1), 0);
    } else {
        vec4 xC = texelFetch(x, gridCellIndex, 0); // Center of texel

        // Fetch neighboring texels
        vec4 xL = texelFetch(x, gridCellIndex - ivec2(1, 0), 0);  // Left
        vec4 xR = texelFetch(x, gridCellIndex + ivec2(1, 0), 0);  // Right
        vec4 xB = texelFetch(x, gridCellIndex - ivec2(0, 1), 0);  // Bottom
        vec4 xT = texelFetch(x, gridCellIndex + ivec2(0, 1), 0);  // Top

        vec4 bC = texelFetch(b, gridCellIndex, 0);

        if (isBoundary(gridCellIndex - ivec2(1, 0))) {
            xL = xC;
        }
        if (isBoundary(gridCellIndex + ivec2(1, 0))) {
            xR = xC;
        }
        if (isBoundary(gridCellIndex - ivec2(0, 1))) {
            xB = xC;
        }
        if (isBoundary(gridCellIndex + ivec2(0, 1))) {
            xT = xC;
        }

        fragColor = (xL + xR + xB + xT + alpha * bC) * rBeta;
    }
}