#version 330 core

uniform sampler2D heightField;
uniform sampler2D prevHeightField;
uniform int gridSize;
uniform float halfrdx;
uniform float timeStep;


in vec2 texCoords;
out vec4 fragColor;


void main() {
    ivec2 gridCellIndex = ivec2(round(texCoords * (gridSize)));  // Convert normalized to integer coordinates

    // Compute Laplacian of the height field
//    float h = texelFetch(heightField, gridCellIndex, 0).x;
    float hC = texture(heightField, texCoords).x;

    float hL = texelFetch(heightField, gridCellIndex - ivec2(1, 0), 0).x;// Left
    float hR = texelFetch(heightField, gridCellIndex + ivec2(1, 0), 0).x;// Right
    float hB = texelFetch(heightField, gridCellIndex - ivec2(0, 1), 0).x;// Bottom
    float hT = texelFetch(heightField, gridCellIndex + ivec2(0, 1), 0).x;// Top

//    float hL = texture(heightField, texCoords + vec2(-1.0 / gridSize, 0)).x;
//    float hR = texture(heightField, texCoords + vec2( 1.0 / gridSize, 0)).x;
//    float hB = texture(heightField, texCoords + vec2(0, -1.0 / gridSize)).x;
//    float hT = texture(heightField, texCoords + vec2(0,  1.0 / gridSize)).x;

    float laplacianH = (hL + hR + hB + hT - 4.0 * hC) / (gridSize * gridSize);

    float c = sqrt(9.81 * 1.0);
    float hPrev = texture(prevHeightField, texCoords).x;

    // Wave propagation step
    float newHeight = ((2.0 * hC - hPrev)/ 2) + (c * c * timeStep * timeStep / (gridSize * gridSize)) * laplacianH;

    // Apply damping (optional)
    newHeight *= 0.98; // Slight damping to prevent infinite oscillations


    fragColor = vec4(newHeight, 0.0, 0.0, 1.0);

}