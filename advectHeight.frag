#version 330 core

uniform sampler2D velocityTexture; //vector field
uniform sampler2D heightField;
uniform float halfrdx;
uniform int gridSize;
uniform float timeStep;

in vec2 texCoords;
out vec4 fragColor;

bool isBoundary(ivec2 gridCellIndex) {
    return gridCellIndex.x == 0 || gridCellIndex.x == gridSize - 1 ||
    gridCellIndex.x == 0 || gridCellIndex.x == gridSize - 1;
}

void main() {
    ivec2 gridCellIndex = ivec2(floor(texCoords * (gridSize - 1)));  // Convert normalized to integer coordinates

//    float h = texelFetch(heightField, gridCellIndex, 0).x;
    float h = texture(heightField, texCoords).x;

    // Fetch neighboring texels
    vec4 vL = texelFetch(velocityTexture, gridCellIndex - ivec2(1, 0), 0);  // Left
    vec4 vR = texelFetch(velocityTexture, gridCellIndex + ivec2(1, 0), 0);  // Right
    vec4 vB = texelFetch(velocityTexture, gridCellIndex - ivec2(0, 1), 0);  // Bottom
    vec4 vT = texelFetch(velocityTexture, gridCellIndex + ivec2(0, 1), 0);  // Top

    float div_hv = h * halfrdx * ((vR.x - vL.x) + (vT.y - vB.y));

    if (!isBoundary(gridCellIndex)) {
//        fragColor = vec4(h - 10 * timeStep * div_hv, 0.0, 0.0, 0.0);
        vec2 newX = texCoords - 20 * timeStep * 2 * halfrdx * texture(velocityTexture, texCoords).xy;
        // Reflect the coordinates back if out of bounds
//        newX = vec2(newX.x < 0.0 ? -newX.x : (newX.x > 1.0 ? 2.0 - newX.x : newX.x),
//        newX.y < 0.0 ? -newX.y : (newX.y > 1.0 ? 2.0 - newX.y : newX.y)
//        );

        fragColor = texture(heightField, newX);
    } else {
        fragColor = vec4(h, 0.0, 0.0, 0.0);  // Preserve height at boundary
    }

}