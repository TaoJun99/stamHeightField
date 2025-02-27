#version 330 core

uniform sampler2D velocityTexture;
uniform sampler2D advectedTexture;
uniform float timestep;
uniform float rdx;

uniform float gridSize;

in vec2 texCoords;

out vec4 fragColor;

void main() {
    // texCoords: [0, 1] --> gridCellndex: [0, gridSize - 1]
    ivec2 gridCellIndex = ivec2(floor(texCoords * gridSize));

    // Boundary cell: Dont advect
    if (gridCellIndex.x == 0) {
        if (gridCellIndex.y == 0 || gridCellIndex.y == gridSize - 1) { // Corner
            fragColor = vec4(0.0, 0.0, 0.0, 0.0);
        } else {
            fragColor = -texelFetch(velocityTexture,gridCellIndex + ivec2(1, 0), 0);
        }
    } else if (gridCellIndex.x == gridSize - 1) {
        if (gridCellIndex.y == 0 || gridCellIndex.y == gridSize - 1) { // Corner
            fragColor = vec4(0.0, 0.0, 0.0, 0.0);
        } else {
            fragColor = -texelFetch(velocityTexture,gridCellIndex - ivec2(1, 0), 0);
        }
    } else if (gridCellIndex.y == 0) {
        fragColor = -texelFetch(velocityTexture,gridCellIndex + ivec2(0, 1), 0);
    } else if (gridCellIndex.y == gridSize - 1) {
        fragColor = -texelFetch(velocityTexture, gridCellIndex - ivec2(0, 1), 0);
    } else { // Inner cells
        vec2 newX = texCoords - timestep * rdx * texture(velocityTexture, texCoords).xy;
        // Reflect the coordinates back if out of bounds
        newX = vec2(newX.x < 0.0 ? -newX.x : (newX.x > 1.0 ? 2.0 - newX.x : newX.x),
                    newX.y < 0.0 ? -newX.y : (newX.y > 1.0 ? 2.0 - newX.y : newX.y)
        );

        vec4 advectedValue = texture(velocityTexture, newX);

        fragColor = advectedValue;
    }


}