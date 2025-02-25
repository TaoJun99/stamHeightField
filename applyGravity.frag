#version 330 core

uniform sampler2D heightField;
uniform sampler2D velocityField;
uniform int gridSize;
uniform float halfrdx;
uniform float timeStep;
uniform bool isHeight;

in vec2 texCoords;
out vec4 fragColor;

bool isBoundary(ivec2 gridCellIndex) {
    return gridCellIndex.x == 0 || gridCellIndex.x == gridSize - 1 ||
    gridCellIndex.y == 0 || gridCellIndex.y == gridSize - 1;
}

void main() {

    ivec2 gridCellIndex = ivec2(floor(texCoords * (gridSize - 1)));  // Convert normalized to integer coordinates

//    float h = texelFetch(heightField, gridCellIndex, 0).x;

    if (isHeight) {
        float h = texture(heightField, texCoords).x;

        if (h > 0) {
            fragColor = vec4(h - 0.01, 0.0, 0.0, 0.0);
        } else if (h < -0.1){
            fragColor = vec4(h, 0.0, 0.0, 0.0);
        } else {
            fragColor = vec4(h, 0.0, 0.0, 0.0);
        }
    } else {

        // Fetch height
        float hL = texelFetch(heightField, gridCellIndex - ivec2(1, 0), 0).x;// Left
        float hR = texelFetch(heightField, gridCellIndex + ivec2(1, 0), 0).x;// Right
        float hB = texelFetch(heightField, gridCellIndex - ivec2(0, 1), 0).x;// Bottom
        float hT = texelFetch(heightField, gridCellIndex + ivec2(0, 1), 0).x;// Top

        // Compute gradient of height field
        vec2 heightGradient = vec2((hR - hL) * halfrdx, (hT - hB) * halfrdx);

        // Fetch velocity
        vec2 v = texelFetch(velocityField, gridCellIndex, 0).xy;

        // Apply gravity force: v_new = v - g * dt * ∇h
        v -= 9.81 * 10 * timeStep * heightGradient;

        fragColor = vec4(v, 0.0, 1.0);
    }



}