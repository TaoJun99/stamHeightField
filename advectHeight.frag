#version 330 core

uniform sampler2D velocityTexture; //vector field
uniform sampler2D heightField;
uniform float halfrdx;
uniform int gridSize;
uniform float timeStep;

in vec2 texCoords;
out vec4 fragColor;

bool isBoundary(vec2 texCoords) {
    return texCoords.x == 0 || texCoords.x == 1 ||
    texCoords.y == 0 || texCoords.y == 1;
}

void main() {
    ivec2 gridCellIndex = ivec2(round(texCoords * (gridSize)));  // Convert normalized to integer coordinates

//    float h = texelFetch(heightField, gridCellIndex, 0).x;
    float h = texture(heightField, texCoords).x;

    // Fetch neighboring texels
//    vec4 vL = texelFetch(velocityTexture, gridCellIndex - ivec2(1, 0), 0);  // Left
//    vec4 vR = texelFetch(velocityTexture, gridCellIndex + ivec2(1, 0), 0);  // Right
//    vec4 vB = texelFetch(velocityTexture, gridCellIndex - ivec2(0, 1), 0);  // Bottom
//    vec4 vT = texelFetch(velocityTexture, gridCellIndex + ivec2(0, 1), 0);  // Top

    vec4 vL = texture(velocityTexture, texCoords + vec2(-1.0 / gridSize, 0));
    vec4 vR = texture(velocityTexture, texCoords + vec2( 1.0 / gridSize, 0));
    vec4 vB = texture(velocityTexture, texCoords + vec2(0, -1.0 / gridSize));
    vec4 vT = texture(velocityTexture, texCoords + vec2(0,  1.0 / gridSize));

    float hL = texture(heightField, texCoords + vec2(-1.0 / gridSize, 0)).x;// Left
    float hR = texture(heightField, texCoords + vec2(1.0 / gridSize, 0)).x;// Right
    float hB = texture(heightField, texCoords + vec2(0, -1.0 / gridSize)).x;// Bottom
    float hT = texture(heightField, texCoords + vec2(0,  1.0 / gridSize)).x;// Top

//    vec2 grad_h = halfrdx * vec2(hR - hL, hT - hB);
    vec2 grad_h = vec2(dFdx(h), dFdy(h));
    vec2 v = texture(velocityTexture, texCoords).xy;

    if (!isBoundary(texCoords)) {
        fragColor = vec4(h - timeStep * dot(v, grad_h), 0.0, 0.0, 0.0);
//        vec2 newX = texCoords - timeStep * 1.0 / float(gridSize) * texture(velocityTexture, texCoords).xy;
        // Reflect the coordinates back if out of bounds
//        newX = vec2(newX.x < 0.0 ? -newX.x : (newX.x > 1.0 ? 2.0 - newX.x : newX.x),
//        newX.y < 0.0 ? -newX.y : (newX.y > 1.0 ? 2.0 - newX.y : newX.y)
//        );

//        fragColor = texture(heightField, newX);
    } else {
        fragColor = vec4(h, 0.0, 0.0, 0.0);  // Preserve height at boundary
    }

//    if (texCoords.x == 0 || texCoords.x == 1 || texCoords.y == 0 || texCoords.y == 1) {
//        fragColor = vec4(h, 0.0, 0.0, 0.0);  // Preserve height at boundary
//    } else {
//        vec2 newX = texCoords - timeStep * 2 * halfrdx * texture(velocityTexture, texCoords).xy;
//        // Reflect the coordinates back if out of bounds
//        newX = vec2(newX.x < 0.0 ? -newX.x : (newX.x > 1.0 ? 2.0 - newX.x : newX.x),
//        newX.y < 0.0 ? -newX.y : (newX.y > 1.0 ? 2.0 - newX.y : newX.y)
//        );
//
//        fragColor = texture(heightField, newX);
//    }

}