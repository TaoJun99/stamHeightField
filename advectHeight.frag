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

    float h = texture(heightField, texCoords).x;

    vec4 v = texture(velocityTexture, texCoords);
    vec4 vL = texture(velocityTexture, texCoords + vec2(-1.0 / gridSize, 0));
    vec4 vR = texture(velocityTexture, texCoords + vec2( 1.0 / gridSize, 0));
    vec4 vB = texture(velocityTexture, texCoords + vec2(0, -1.0 / gridSize));
    vec4 vT = texture(velocityTexture, texCoords + vec2(0,  1.0 / gridSize));

    float hL = texture(heightField, texCoords + vec2(-1.0 / gridSize, 0)).x;// Left
    float hR = texture(heightField, texCoords + vec2(1.0 / gridSize, 0)).x;// Right
    float hB = texture(heightField, texCoords + vec2(0, -1.0 / gridSize)).x;// Bottom
    float hT = texture(heightField, texCoords + vec2(0,  1.0 / gridSize)).x;// Top


    if (!isBoundary(texCoords)) {
//        fragColor = vec4(h - timeStep * dot(v, grad_h), 0.0, 0.0, 0.0); // advection: v dot grad_h

        float h1, h2;

        if (v.x > 0) {
            h1 = h;
        } else {
            h1 = hR;
        }

        if (vL.x > 0) {
            h2 = hL;
        } else {
            h2 = h;
        }

        float h3, h4;
        if (v.y > 0) {
            h3 = h;
        } else {
            h3 = hT;
        }

        if (vB.y > 0) {
            h4 = hB;
        } else {
            h4 = h;
        }


        float dh_dt = - 2 * halfrdx * ((h1 * v.x - h2 * vL.x) + (h3 * v.y - h4 * vB.y));
//        fragColor = vec4(h + timeStep * dh_dt, 0.0, 0.0, 0.0);

        vec2 grad_h = vec2((hR - hL) * halfrdx, (hT - hB) * halfrdx);

        vec2 v = texture(velocityTexture, texCoords).xy;

        float dv_dx = (vR.x - vL.x) * halfrdx;
        float dv_dy = (vT.y - vB.y) * halfrdx;

        float div_v = dv_dx + dv_dy;

        fragColor = vec4(h - timeStep * (dot(v, grad_h) + h * div_v), 0.0, 0.0, 0.0);



    } else {
        fragColor = vec4(h, 0.0, 0.0, 0.0);  // Preserve height at boundary
    }

}