#version 330 core

out vec4 fragColor;

in vec2 texCoords;

uniform sampler2D velocityTexture;
uniform vec3 forcePos;      // Normalized
uniform vec2 forceDir;      // Direction of the force
uniform float forceRadius;  // Radius of the force application
uniform float forceStrength;
uniform int gridSize;
uniform float size;

void main() {
    ivec2 gridCellIndex = ivec2(floor(texCoords * (gridSize - 1)));

    if (gridCellIndex.x == 0 || gridCellIndex.x == gridSize - 1 ||
    gridCellIndex.y == 0 || gridCellIndex.y == gridSize - 1) { //Boundary: dont apply force
        fragColor = texture(velocityTexture, texCoords);
    } else {
        // Calculate distance from the mouse position
        float distance = length(texCoords - forcePos.xz);

        // Apply force within the radius
        if (distance < forceRadius) {
            vec2 direction = normalize(texCoords - forcePos.xz);


            float influence = exp(-distance * distance / (2.0 * forceRadius * forceRadius));
            vec2 currentVelocity = texture(velocityTexture, texCoords).xy;
            vec2 newVelocity = currentVelocity + influence * direction * forceStrength; // radial direction


            fragColor = vec4(newVelocity, 0.0, 1.0);

//            vec2 direction = normalize(texCoords - forcePos.xz + vec2(0.01)); // Avoid instability
//            float sigma = 0.5 * forceRadius;
//            float influence = exp(-distance * distance / (2.0 * sigma * sigma));
//
//            vec2 currentVelocity = textureLod(velocityTexture, texCoords, 0.0).xy;
//            vec2 newVelocity = mix(currentVelocity, currentVelocity + influence * direction * forceStrength, 0.1); // Smooth blending

            fragColor = vec4(newVelocity, 0.0, 1.0);
        } else {
            fragColor = texture(velocityTexture, texCoords);
        }
    }

}
