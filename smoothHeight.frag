#version 330 core

uniform sampler2D heightField;
uniform int gridSize;


in vec2 texCoords;
out vec4 fragColor;


void main() {
    // Gaussian kernel weights (approximated)
    const float kernel[9] = float[](
    1.0, 2.0, 1.0,
    2.0, 4.0, 2.0,
    1.0, 2.0, 1.0
    );
    const float weightSum = 16.0; // Sum of weights

    float hC = texture(heightField, texCoords).x;

    float hL = texture(heightField, texCoords + vec2( -1.0 / gridSize, 0)).x;// Left
    float hR = texture(heightField, texCoords + vec2(1.0 / gridSize, 0)).x;// Right
    float hB = texture(heightField, texCoords + vec2(0,-1.0 / gridSize)).x;// Bottom
    float hT = texture(heightField, texCoords + vec2(0, 1.0 / gridSize)).x;// Top

    float hTL = texture(heightField, texCoords + vec2(1.0 / gridSize, -1.0 / gridSize)).x;// Top left
    float hTR = texture(heightField, texCoords + vec2(1.0 / gridSize, 1.0 / gridSize)).x;// Top Right
    float hBL = texture(heightField, texCoords + vec2(-1.0 / gridSize,-1.0 / gridSize)).x;// Bottom left
    float hBR = texture(heightField, texCoords + vec2(-1.0 / gridSize, 1.0 / gridSize)).x;// Bottom right

    // Apply Gaussian kernel
    float hSmooth = (hTL + 2.0 * hT + hTR +
    2.0 * hL + 4.0 * hC + 2.0 * hR +
    hBL + 2.0 * hB + hBR) / weightSum;

    fragColor = vec4(hSmooth, 0.0, 0.0, 0.0);
}

