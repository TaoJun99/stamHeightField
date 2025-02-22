#version 330 core

out vec4 fragColor;

in vec2 texCoords;

uniform sampler2D velocityTexture;
uniform vec3 forcePos;      // World space position
uniform vec2 forceDir;      // Direction of the force
uniform float forceRadius;  // Radius of the force application
uniform float forceStrength;
uniform int gridSize;
uniform float size;

void main() {
    ivec2 gridCellIndex = ivec2(floor(texCoords * (gridSize - 1)));

    if (gridCellIndex.x == 0 || gridCellIndex.x == gridSize - 1 ||
    gridCellIndex.x == 0 || gridCellIndex.x == gridSize - 1) { //Boundary: dont apply force
        fragColor = texture(velocityTexture, texCoords);
    } else {
        // Calculate distance from the mouse position
        float distance = length(texCoords - forcePos.xz);

        // Apply force within the radius
        if (distance < forceRadius) {
            float influence = exp(-distance * distance / (2.0 * forceRadius * forceRadius));
            vec2 currentVelocity = texture(velocityTexture, texCoords).xy;
//            vec2 newVelocity = currentVelocity + influence * forceDir * forceStrength;
            vec2 newVelocity = currentVelocity + influence * normalize(texCoords) * forceStrength; // radial direction

            fragColor = vec4(newVelocity, 0.0, 1.0);
        } else {
            fragColor = texture(velocityTexture, texCoords);
        }
    }

}
