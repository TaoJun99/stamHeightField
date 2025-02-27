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

        // Compute fluxes (h * v)
        float fluxR = hR * vR.x;
        float fluxL = hL * vL.x;
        float fluxT = hT * vT.y;
        float fluxB = hB * vB.y;

        // Compute divergence of flux
        float divFlux = 2 * halfrdx * ((fluxR - fluxL)  + (fluxT - fluxB));


        float dh_dt = - 2 * halfrdx * ((h1 * v.x - h2 * vL.x) + (h3 * v.y - h4 * vB.y));
//        fragColor = vec4(h + timeStep * dh_dt, 0.0, 0.0, 0.0);
//        fragColor = vec4(h - timeStep * divFlux, 0.0, 0.0, 0.0);

        vec2 grad_h = vec2(dFdx(h), dFdy(h));
        vec2 v = texture(velocityTexture, texCoords).xy;

        float dvx_dx = dFdx(v.x); // Derivative of velocity in x direction (partial v_x / partial x)
        float dvy_dy = dFdy(v.y); // Derivative of velocity in y direction (partial v_y / partial y)
        float div_v = dvx_dx + dvy_dy;

//        fragColor = vec4(h - timeStep * (dot(v, grad_h) + h * div_v), 0.0, 0.0, 0.0);

        vec2 first = dFdx(h * v);
        float second = dFdy(h * v.y);
        fragColor = vec4(h + timeStep * (first.x + first.y), 0.0, 0.0, 0.0);


    } else {
        fragColor = vec4(h, 0.0, 0.0, 0.0);  // Preserve height at boundary
    }

}