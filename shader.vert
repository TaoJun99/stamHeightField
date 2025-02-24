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

vec3 computeSurfaceNormal() {
    ivec2 texel = ivec2(floor(texCoords * float(gridSize - 1)));

    float delta_x = 1.0 / gridSize;

    // Get the height values at the current texel and its neighbors (using central difference)
    float hL = texelFetch(inputTexture, texel - ivec2(1, 0), 0).x; // Left neighbor
    float hR = texelFetch(inputTexture, texel + ivec2(1, 0), 0).x; // Right neighbor
    float hD = texelFetch(inputTexture, texel - ivec2(0, 1), 0).x; // Down neighbor
    float hU = texelFetch(inputTexture, texel + ivec2(0, 1), 0).x; // Up neighbor

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
    vec3 position = vec3(aPos.x, height, aPos.z);

    gl_Position = projection * view * model * vec4(position, 1.0);

    ecNormal = normalize(mat3(transpose(inverse(view * model))) * computeSurfaceNormal());
    ecPosition = vec3(view * model * vec4(position, 1.0));
}