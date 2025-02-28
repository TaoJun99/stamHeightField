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
    vec4 vR = texture(velocityTexture, texCoords + vec2(1.0 / gridSize, 0));
    vec4 vB = texture(velocityTexture, texCoords + vec2(0, -1.0 / gridSize));
    vec4 vT = texture(velocityTexture, texCoords + vec2(0,  1.0 / gridSize));

    vec4 vTL = texture(velocityTexture, texCoords + vec2(1.0 / gridSize, -1.0 / gridSize));
    vec4 vTR = texture(velocityTexture, texCoords + vec2(1.0 / gridSize, 1.0 / gridSize));
    vec4 vBL = texture(velocityTexture, texCoords + vec2(-1.0 / gridSize,-1.0 / gridSize));
    vec4 vBR = texture(velocityTexture, texCoords + vec2(-1.0 / gridSize, 1.0 / gridSize));


//    vec4 vL = texture(velocityTexture, texCoords + 0.5 * vec2(-1.0 / gridSize, 0));
//    vec4 vR = texture(velocityTexture, texCoords + 0.5 * vec2(1.0 / gridSize, 0));
//    vec4 vB = texture(velocityTexture, texCoords + 0.5 * vec2(0, -1.0 / gridSize));
//    vec4 vT = texture(velocityTexture, texCoords + 0.5 * vec2(0,  1.0 / gridSize));

    float hL = texture(heightField, texCoords + vec2( -1.0 / gridSize, 0)).x;// Left
    float hR = texture(heightField, texCoords + vec2(1.0 / gridSize, 0)).x;// Right
    float hB = texture(heightField, texCoords + vec2(0,-1.0 / gridSize)).x;// Bottom
    float hT = texture(heightField, texCoords + vec2(0, 1.0 / gridSize)).x;// Top

    float hTL = texture(heightField, texCoords + vec2(1.0 / gridSize, -1.0 / gridSize)).x;// Top left
    float hTR = texture(heightField, texCoords + vec2(1.0 / gridSize, 1.0 / gridSize)).x;// Top Right
    float hBL = texture(heightField, texCoords + vec2(-1.0 / gridSize,-1.0 / gridSize)).x;// Bottom left
    float hBR = texture(heightField, texCoords + vec2(-1.0 / gridSize, 1.0 / gridSize)).x;// Bottom right


    if (!isBoundary(texCoords)) {
        float h1, h2;

        if (vR.x > 0) {
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
        if (vT.y > 0) {
            h3 = h;
        } else {
            h3 = hT;
        }

        if (vB.y > 0) {
            h4 = hB;
        } else {
            h4 = h;
        }


//        float dh_dt = 2 * halfrdx * ((h1 * vR.x - h2 * vL.x) + (h3 * vT.y - h4 * vB.y));
//        fragColor = vec4(max(0, h - timeStep * dh_dt), 0.0, 0.0, 0.0);


        vec2 grad_h = vec2((hR - hL) * halfrdx, (hT - hB) * halfrdx);
//        vec2 v = texture(velocityTexture, texCoords).xy;

        float dv_dx = (vR.x - vL.x) * halfrdx;
        float dv_dy = (vT.y - vB.y) * halfrdx;

        float div_v = dv_dx + dv_dy;


//        float grad_h_x = (hR - h) * halfrdx;
//        float grad_h_y = (hT - hB) * halfrdx;
//
//        float grad_h_diag1 = (hTR - hBL) * halfrdx;
//        float grad_h_diag2 = (hTL - hBR) * halfrdx;
//
//        vec2 grad_h = vec2((grad_h_x + grad_h_diag1 + grad_h_diag2) * 0.25,
//                           (grad_h_y + grad_h_diag1 + grad_h_diag2) * 0.25);
//
//
//
//        float div_v_x = (vR.x - vL.x) * halfrdx;
//        float div_v_y = (vT.y - vB.y) * halfrdx;
//        // Diagonal divergences (in both diagonal directions)
//        float div_v_diag1 = (vTR.x - vBL.x + vTR.y - vBL.y) * halfrdx;
//        float div_v_diag2 = (vTL.x - vBR.x + vTL.y - vBR.y) * halfrdx;
//        // Averaging the divergences
//        float div_v = (div_v_x + div_v_y + div_v_diag1 + div_v_diag2) * 0.25;


        fragColor = vec4(h - timeStep * (dot(v.xy, grad_h) + h * div_v), 0.0, 0.0, 0.0);


    } else {
        fragColor = vec4(5.0, 0.0, 0.0, 0.0);  // Preserve height at boundary
    }

}