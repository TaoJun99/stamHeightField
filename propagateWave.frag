#version 330 core

uniform sampler2D heightField;
uniform sampler2D prevHeightField;
uniform int gridSize;
uniform float halfrdx;
uniform float timeStep;


in vec2 texCoords;
out vec4 fragColor;


void main() {
    ivec2 gridCellIndex = ivec2(floor(texCoords * (gridSize - 1)));  // Convert normalized to integer coordinates

    float newHeight;
//    if (gridCellIndex.x == 0 || gridCellIndex.x == gridSize - 1) {
//        newHeight = -texture(heightField, texCoords + vec2(-1.0 / gridSize, 0)).x;  // Reflect horizontally
//    } else if (gridCellIndex.y == 0 || gridCellIndex.y == gridSize - 1) {
//        newHeight = -texture(heightField, texCoords + vec2(0, -1.0 / gridSize)).x;  // Reflect vertically
//    } else {

        // Compute Laplacian of the height field
        float hC = texture(heightField, texCoords).x;

        float hL = texture(heightField, texCoords + vec2(-1.0 / gridSize, 0)).x;
        float hR = texture(heightField, texCoords + vec2(1.0 / gridSize, 0)).x;
        float hB = texture(heightField, texCoords + vec2(0, -1.0 / gridSize)).x;
        float hT = texture(heightField, texCoords + vec2(0, 1.0 / gridSize)).x;

        float laplacianH = (hL + hR + hB + hT - 4.0 * hC) * gridSize * gridSize;

        float c = sqrt(9.81 * 0.7);
        float hPrev = texture(prevHeightField, texCoords).x;

        // Wave propagation step
        newHeight = (2 * hC - hPrev) + (c * c * timeStep * timeStep) * laplacianH;

        // Apply damping (optional)
        newHeight *= 0.98;// Slight damping to prevent infinite oscillations
//    }

    fragColor = vec4(newHeight, 0.0, 0.0, 1.0);

}