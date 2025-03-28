#version 330 core

layout (location = 0) in vec3 aPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform int gridSize;
uniform float size;
uniform sampler2D inputTexture;

out vec2 texCoords;
out vec3 ecNormal;
out vec3 ecPosition;
out vec2 floorTexCoords;

vec3 computeSurfaceNormal() {
    ivec2 texel = ivec2(round(texCoords * float(gridSize - 1)));

    float delta_x = 1.0 / gridSize;

    // Get the height values at the current texel and its neighbors (using central difference)
//    float hL = texelFetch(inputTexture, texel - ivec2(1, 0), 0).x; // Left neighbor
//    float hR = texelFetch(inputTexture, texel + ivec2(1, 0), 0).x; // Right neighbor
//    float hD = texelFetch(inputTexture, texel - ivec2(0, 1), 0).x; // Down neighbor
//    float hU = texelFetch(inputTexture, texel + ivec2(0, 1), 0).x; // Up neighbor

    float hL = texture(inputTexture, texCoords + vec2(-1.0 / gridSize, 0)).x;
    float hR = texture(inputTexture, texCoords + vec2( 1.0 / gridSize, 0)).x;
    float hD = texture(inputTexture, texCoords + vec2(0, -1.0 / gridSize)).x;
    float hU = texture(inputTexture, texCoords + vec2(0,  1.0 / gridSize)).x;

    // Compute the partial derivatives using central difference
    float dHdx = (hR - hL) / (2.0 * delta_x); // Gradient in the x direction
    float dHdz = (hU - hD) / (2.0 * delta_x); // Gradient in the z direction

    // Construct the normal vector (negate the gradient and set z to 1)
    vec3 normal = normalize(vec3(-dHdx, 1.0, -dHdz));

    return normal; //world-space normal
}

void main() {
    texCoords = (aPos.xz + size / 2) / size;

    float height = texture(inputTexture, texCoords).x;
//    float height = length(texture(inputTexture, texCoords).xy);
    vec3 position = vec3(aPos.x, height, aPos.z);

    gl_Position = projection * view * model * vec4(position, 1.0);

    ecNormal = normalize(mat3(transpose(inverse(view * model))) * computeSurfaceNormal());
    ecPosition = vec3(view * model * vec4(position, 1.0));

    floorTexCoords = (aPos.xz) * 0.1;


    mat2 rotation = mat2(1.0, 0.0,  // Undo shear effect
    -0.8,  1.0);
    floorTexCoords = rotation * floorTexCoords;
}