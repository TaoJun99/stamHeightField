#version 330 core

out vec4 fragColor;

in vec2 texCoords;

uniform sampler2D heightTexture;
uniform vec3 forcePos;      // Normalized
uniform vec2 forceDir;      // Direction of the force
uniform float forceRadius;  // Radius of the force application
uniform float forceStrength;
uniform int gridSize;
uniform float size;

void main() {
    ivec2 gridCellIndex = ivec2(round(texCoords * (gridSize)));

    if (texCoords.x == 0 || texCoords.x == 1 ||
    texCoords.y == 0 || texCoords.y ==  1) { //Boundary: dont apply force
        fragColor = texture(heightTexture, texCoords);
    } else {
        // Calculate distance from the mouse position
        float distance = length(texCoords - forcePos.xz);

        if (distance < forceRadius) {
            float normalizedDistance = distance / forceRadius;
            float influence = smoothstep(1.0, 0.0, normalizedDistance);

            float currentHeight = texture(heightTexture, texCoords).x;
            float newHeight = mix(currentHeight, currentHeight - forceStrength, influence); // Use mix() for smooth interpolation

//            float influence = exp(-distance * distance / (2.0 * forceRadius * forceRadius));
//            float currentHeight = texture(heightTexture, texCoords).x;
//            float newHeight = currentHeight - influence * forceStrength; // radial direction

            fragColor = vec4(newHeight, 0.0, 0.0, 1.0);
        } else {
            fragColor = texture(heightTexture, texCoords);
        }


    }

}