#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <glm/gtc/type_ptr.hpp>
#include "Camera.h"
#include <vector>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"


GLuint waterVAO, waterVBO, waterEBO, waterShader;
GLuint skyboxVAO, skyboxVBO, skyboxShader;
GLuint skyBoxtid;
//GLuint projectionLoc, viewLoc, modelLoc;
glm::mat4 projection, view;
glm::vec3 cameraPos = glm::vec3(0.0f, 13.0f, 2.5f);
float cameraWidth = 800.0f;
float cameraHeight = 600.0f;
Camera camera(cameraWidth, cameraHeight, cameraPos);
int waterPlaneIndexCount;
GLuint oceanHeightTexture;
GLuint prevHeightTexture;

GLuint quadVAO, quadVBO, quadEBO;

GLuint advectShaderProgram;
GLuint jacobiShaderProgram;
GLuint applyForceShaderProgram;
GLuint divergenceShaderProgram;
GLuint gradientSubtractShaderProgram;
GLuint velocityTexture;
GLuint pressureTexture;
GLuint jacobiTexture1;
GLuint jacobiTexture2;
GLuint framebuffer;

GLuint advectHeightShaderProgram;
GLuint displaceHeightShaderProgram;
GLuint applyGravityShaderProgram;
GLuint propagateWaveShaderProgram;
GLuint velocityIntegrationShaderProgram;
GLuint initHeightShaderProgram;
GLuint smoothHeightShaderProgram;

float timeStep = 0.5;

// Light info.
const GLfloat lightAmbient[] = { 0.1f, 0.2f, 0.3f, 1.0f };
const GLfloat lightDiffuse[] = { 1.0f, 1.0f, 1.0f, 1.0f };
const GLfloat lightSpecular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
const GLfloat lightPosition[4] = {0.0f, 10.0f, 0.0f, 0.0f }; // Given in eye space

// Grid size
const int gridSize = 1000; // Number of segments in each direction
const float size = 20.0f;  // Size of the plane

std::vector<GLfloat> zeroData(gridSize * gridSize * 4, 0.0f);

float yPlaneHeight = 10.0;

float quadVertices[] = {
        -1.0f, -1.0f,
        1.0f, -1.0f,
        1.0f,  1.0f,
        -1.0f,  1.0f
};

unsigned int quadIndices[] = {
        0, 1, 2,
        2, 3, 0
};

// Function to read shader source from file
std::string readShaderSource(const std::string& filePath) {
    std::ifstream file(filePath);
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// Function to compile shader
GLuint compileShader(const std::string& source, GLenum shaderType) {
    GLuint shader = glCreateShader(shaderType);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    // Check for compilation errors
    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "Shader compilation failed: " << infoLog << std::endl;
    }

    return shader;
}

// Function to create shader program
GLuint createShaderProgram(const std::string& vertexPath, const std::string& fragmentPath) {
    std::string vertexSource = readShaderSource(vertexPath);
    std::string fragmentSource = readShaderSource(fragmentPath);

    GLuint vertexShader = compileShader(vertexSource, GL_VERTEX_SHADER);
    GLuint fragmentShader = compileShader(fragmentSource, GL_FRAGMENT_SHADER);

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    // Check for linking errors
    int success;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
        std::cerr << "Shader program linking failed: " << infoLog << std::endl;
    }

    // Clean up shaders
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}



void generatePlane(float** vertices, unsigned int** indices, int* indexCount) {
    // Generate vertices
    *vertices = new float[(gridSize + 1) * (gridSize + 1) * 3]; // 3 for x, y, z
    for (int z = 0; z <= gridSize; ++z) {
        for (int x = 0; x <= gridSize; ++x) {
            (*vertices)[(z * (gridSize + 1) + x) * 3 + 0] = (x / (float)gridSize) * size - size / 2; // x
            (*vertices)[(z * (gridSize + 1) + x) * 3 + 1] = yPlaneHeight; // y (initially flat)
            (*vertices)[(z * (gridSize + 1) + x) * 3 + 2] = (z / (float)gridSize) * size - size / 2; // z
        }
    }

    // Generate indices
    *indexCount = gridSize * gridSize * 6; // Each quad consists of 2 triangles
    *indices = new unsigned int[*indexCount];
    int offset = 0;
    for (int z = 0; z < gridSize; ++z) {
        for (int x = 0; x < gridSize; ++x) {
            (*indices)[offset++] = (z * (gridSize + 1)) + x;         // Top left
            (*indices)[offset++] = ((z + 1) * (gridSize + 1)) + x;   // Bottom left
            (*indices)[offset++] = (z * (gridSize + 1)) + (x + 1);   // Top right

            (*indices)[offset++] = ((z + 1) * (gridSize + 1)) + x;   // Bottom left
            (*indices)[offset++] = ((z + 1) * (gridSize + 1)) + (x + 1); // Bottom right
            (*indices)[offset++] = (z * (gridSize + 1)) + (x + 1);   // Top right
        }
    }
}

void setupWater() {
    // Vertex data for a detailed grid plane
    float* vertices;
    unsigned int* indices;

    generatePlane(&vertices, &indices, &waterPlaneIndexCount);

    glGenVertexArrays(1, &waterVAO);
    glGenBuffers(1, &waterVBO);
    glGenBuffers(1, &waterEBO);

    // Bind VAO
    glBindVertexArray(waterVAO);

    // Bind and set VBO
    glBindBuffer(GL_ARRAY_BUFFER, waterVBO);
    glBufferData(GL_ARRAY_BUFFER, (gridSize + 1) * (gridSize + 1) * 3 * sizeof(float), vertices, GL_STATIC_DRAW);

    // Bind and set EBO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, waterEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, waterPlaneIndexCount * sizeof(unsigned int), indices, GL_STATIC_DRAW);

    // Configure vertex attributes
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Unbind VAO
    glBindVertexArray(0);

    delete[] vertices;
    delete[] indices;


    glUseProgram(initHeightShaderProgram);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, oceanHeightTexture);

    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, oceanHeightTexture, 0);

    glViewport(0, 0, gridSize, gridSize);
//    glClear(GL_COLOR_BUFFER_BIT);

    glBindVertexArray(quadVAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

}

void drawWater() {
    glUseProgram(waterShader);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, oceanHeightTexture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, velocityTexture);

    // Matrices
    glm::mat4 model = glm::mat4(1.0f);  // Identity matrix (no transformation)

    GLuint modelLoc = glGetUniformLocation(waterShader, "model");
    GLuint viewLoc = glGetUniformLocation(waterShader, "view");
    GLuint projectionLoc = glGetUniformLocation(waterShader, "projection");
    GLuint lightPosLoc = glGetUniformLocation(waterShader, "LightPosition");
    GLuint lightAmbientLoc = glGetUniformLocation(waterShader, "LightAmbient");
    GLuint lightDiffuseLoc = glGetUniformLocation(waterShader, "LightDiffuse");
    GLuint lightSpecularLoc = glGetUniformLocation(waterShader, "LightSpecular");
    GLuint textureLoc = glGetUniformLocation(waterShader, "inputTexture");
    GLuint sizeLoc = glGetUniformLocation(waterShader, "size");
    GLuint gridSizeLoc = glGetUniformLocation(waterShader, "gridSize");
    GLuint envMapLoc = glGetUniformLocation(waterShader, "envMap");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

    glUniform4f(lightPosLoc, lightPosition[0], lightPosition[1], lightPosition[2], lightPosition[3]);
    glUniform4f(lightAmbientLoc, lightAmbient[0], lightAmbient[1], lightAmbient[2], lightAmbient[3]);
    glUniform4f(lightDiffuseLoc, lightDiffuse[0], lightDiffuse[1], lightDiffuse[2], lightDiffuse[3]);
    glUniform4f(lightSpecularLoc, lightSpecular[0], lightSpecular[1], lightSpecular[2], lightSpecular[3]);
    glUniform1i(textureLoc, 0);
    glUniform1f(sizeLoc, size);
    glUniform1i(gridSizeLoc, gridSize);
    glUniform1i(envMapLoc, 4);

    glBindVertexArray(waterVAO);
    glBindTexture(GL_TEXTURE_CUBE_MAP, skyBoxtid);
    glDrawElements(GL_TRIANGLES, waterPlaneIndexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

//    glBindVertexArray(quadVAO);
//    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
//    glBindVertexArray(0);

}


void setUpEnvMap() {
    const int numImages = 6;
    const GLenum texUnit = GL_TEXTURE4;

    // Cubemap images' filenames.
    const char *cubemapFile[numImages] = {
            "../images/right.png", "../images/left.png",
            "../images/top.png", "../images/bottom.png",
            "../images/front.png", "../images/back.png"
    };


    GLuint target[numImages] = {
            GL_TEXTURE_CUBE_MAP_POSITIVE_X, GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
            GL_TEXTURE_CUBE_MAP_POSITIVE_Y, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
            GL_TEXTURE_CUBE_MAP_POSITIVE_Z, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
    };


    glActiveTexture(texUnit);
    glGenTextures(1, &skyBoxtid);
    glBindTexture(GL_TEXTURE_CUBE_MAP, skyBoxtid);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

    // Enable flipping of images vertically when read in.
    // This is to follow OpenGL's image coordinate system, i.e. bottom-leftmost is (0, 0).
//    stbi_set_flip_vertically_on_load(true);

    // Read texture images from files.
    for (int t = 0; t < numImages; t++) {

        int imgWidth, imgHeight, numComponents;

        GLubyte *imgData = stbi_load(cubemapFile[t], &imgWidth, &imgHeight, &numComponents, 0);

        if (imgData == NULL) {
            fprintf(stderr, "Error: Fail to read image file %s.\n", cubemapFile[t]);
            exit(EXIT_FAILURE);
        }
        printf("%s (%d x %d, %d components)\n", cubemapFile[t], imgWidth, imgHeight, numComponents);

        if (numComponents == 1) {
            glTexImage2D(target[t], 0, GL_R8, imgWidth, imgHeight, 0,
                         GL_RED, GL_UNSIGNED_BYTE, imgData);
        }
        else if (numComponents == 3) {
            glTexImage2D(target[t], 0, GL_RGB8, imgWidth, imgHeight, 0,
                         GL_RGB, GL_UNSIGNED_BYTE, imgData);
        }
        else if (numComponents == 4) {
            glTexImage2D(target[t], 0, GL_RGBA8, imgWidth, imgHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, imgData);
        }
        else {
            fprintf(stderr, "Error: Unexpected image format.\n");
            exit(EXIT_FAILURE);
        }

        stbi_image_free(imgData);
    }

    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
}

const float skyBoxSize = 2.0f;

void setupSkybox() {
    // Define the vertices for a cube
    float skyboxVertices[] = {
            // Positions
            -1.0f * skyBoxSize,  1.0f * skyBoxSize, -1.0f * skyBoxSize,
            -1.0f * skyBoxSize, -1.0f * skyBoxSize, -1.0f * skyBoxSize,
            1.0f * skyBoxSize, -1.0f * skyBoxSize, -1.0f * skyBoxSize,
            1.0f * skyBoxSize, -1.0f * skyBoxSize, -1.0f * skyBoxSize,
            1.0f * skyBoxSize,  1.0f * skyBoxSize, -1.0f * skyBoxSize,
            -1.0f * skyBoxSize,  1.0f * skyBoxSize, -1.0f * skyBoxSize,

            -1.0f * skyBoxSize, -1.0f * skyBoxSize,  1.0f * skyBoxSize,
            -1.0f * skyBoxSize, -1.0f * skyBoxSize, -1.0f * skyBoxSize,
            -1.0f * skyBoxSize,  1.0f * skyBoxSize, -1.0f * skyBoxSize,
            -1.0f * skyBoxSize,  1.0f * skyBoxSize, -1.0f * skyBoxSize,
            -1.0f * skyBoxSize,  1.0f * skyBoxSize,  1.0f * skyBoxSize,
            -1.0f * skyBoxSize, -1.0f * skyBoxSize,  1.0f * skyBoxSize,

            1.0f * skyBoxSize, -1.0f * skyBoxSize, -1.0f * skyBoxSize,
            1.0f * skyBoxSize, -1.0f * skyBoxSize,  1.0f * skyBoxSize,
            1.0f * skyBoxSize,  1.0f * skyBoxSize,  1.0f * skyBoxSize,
            1.0f * skyBoxSize,  1.0f * skyBoxSize,  1.0f * skyBoxSize,
            1.0f * skyBoxSize,  1.0f * skyBoxSize, -1.0f * skyBoxSize,
            1.0f * skyBoxSize, -1.0f * skyBoxSize, -1.0f * skyBoxSize,

            -1.0f * skyBoxSize, -1.0f * skyBoxSize,  1.0f * skyBoxSize,
            -1.0f * skyBoxSize,  1.0f * skyBoxSize,  1.0f * skyBoxSize,
            1.0f * skyBoxSize,  1.0f * skyBoxSize,  1.0f * skyBoxSize,
            1.0f * skyBoxSize,  1.0f * skyBoxSize,  1.0f * skyBoxSize,
            1.0f * skyBoxSize, -1.0f * skyBoxSize,  1.0f * skyBoxSize,
            -1.0f * skyBoxSize, -1.0f * skyBoxSize,  1.0f * skyBoxSize,

            -1.0f * skyBoxSize,  1.0f * skyBoxSize, -1.0f * skyBoxSize,
            1.0f * skyBoxSize,  1.0f * skyBoxSize, -1.0f * skyBoxSize,
            1.0f * skyBoxSize,  1.0f * skyBoxSize,  1.0f * skyBoxSize,
            1.0f * skyBoxSize,  1.0f * skyBoxSize,  1.0f * skyBoxSize,
            -1.0f * skyBoxSize,  1.0f * skyBoxSize,  1.0f * skyBoxSize,
            -1.0f * skyBoxSize,  1.0f * skyBoxSize, -1.0f * skyBoxSize,

            -1.0f * skyBoxSize, -1.0f * skyBoxSize,  1.0f * skyBoxSize,
            -1.0f * skyBoxSize, -1.0f * skyBoxSize, -1.0f * skyBoxSize,
            1.0f * skyBoxSize, -1.0f * skyBoxSize, -1.0f * skyBoxSize,
            1.0f * skyBoxSize, -1.0f * skyBoxSize, -1.0f * skyBoxSize,
            1.0f * skyBoxSize, -1.0f * skyBoxSize,  1.0f * skyBoxSize,
            -1.0f * skyBoxSize, -1.0f * skyBoxSize,  1.0f * skyBoxSize
    };


    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);

    glBindVertexArray(skyboxVAO);

    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), skyboxVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void drawSkybox() {
    if (skyBoxtid == 0) {
        std::cerr << "Error: skyBox texture not generated properly!" << std::endl;
    }


    glUseProgram(skyboxShader);

    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_CUBE_MAP, skyBoxtid);

    GLuint viewLoc = glGetUniformLocation(skyboxShader, "view");
    GLuint projectionLoc = glGetUniformLocation(skyboxShader, "projection");
    GLuint skyBoxLoc = glGetUniformLocation(skyboxShader, "skybox");
    glUniform1i(skyBoxLoc, 4);

    // Remove translation
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(glm::mat4(glm::mat3(view))));
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

    glBindVertexArray(skyboxVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

void advect(GLuint texture) {
    glUseProgram(advectShaderProgram);

    // Generate texture to store intermediate results

    GLuint outputTexture;
    glGenTextures(1, &outputTexture);
    glBindTexture(GL_TEXTURE_2D, outputTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, gridSize, gridSize, 0, GL_RGBA, GL_FLOAT, zeroData.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Bind the velocity and dye textures

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, velocityTexture);

    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, outputTexture);


    // Set the uniform variables
    GLuint timestepLoc = glGetUniformLocation(advectShaderProgram, "timestep");
    GLuint rdxLoc = glGetUniformLocation(advectShaderProgram, "rdx");

    GLuint velocityTextureLoc = glGetUniformLocation(advectShaderProgram, "velocityTexture");

    GLuint gridSizeLoc = glGetUniformLocation(advectShaderProgram, "gridSize");

    glUniform1f(timestepLoc, timeStep);
    glUniform1f(rdxLoc, 1.0 / gridSize);
    glUniform1i(velocityTextureLoc, 1);
    glUniform1f(gridSizeLoc, gridSize);


    // Render texture to framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, outputTexture, 0);

    glViewport(0, 0, gridSize, gridSize);
//    glClear(GL_COLOR_BUFFER_BIT);

    glBindVertexArray(quadVAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);


    if (texture == oceanHeightTexture) {
        glBindTexture(GL_TEXTURE_2D, oceanHeightTexture);
        glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, gridSize, gridSize);
    } else if (texture == velocityTexture) {
        glBindTexture(GL_TEXTURE_2D, velocityTexture);
        glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, gridSize, gridSize);
    }


    // Unbind the framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

//    glDeleteFramebuffers(1, &tempframebuffer);
    glDeleteTextures(1, &outputTexture);
}

void jacobi(GLuint outputTexture, GLuint xLoc) {
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glViewport(0,0,gridSize, gridSize);

    // 1st iteration
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, jacobiTexture1, 0);

    if (outputTexture == velocityTexture) {
        glUniform1i(xLoc, 1);
    } else if (outputTexture == pressureTexture) {
        glUniform1i(xLoc, 2);
    }

    glBindVertexArray(quadVAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    int NO_OF_ITERATIONS = 50;
    GLuint currTexture;

    for (int i = 0; i < NO_OF_ITERATIONS; i++) {
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

        // Alternate between two textures to read & write
        if (i % 2 == 0) { // Multiple of 2 - input: jacobiTexture1, output: jacobiTexture2
            currTexture = jacobiTexture2;
            glActiveTexture(GL_TEXTURE4);
            glBindTexture(GL_TEXTURE_2D, currTexture);
            // Bind output texture to framebuffer
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, currTexture, 0);
            // Input texture
            glUniform1i(xLoc, 3);
        } else {// input: jacobiTexture2, output: jacobiTexture1
            currTexture = jacobiTexture1;
            // Bind output texture to framebuffer
            glActiveTexture(GL_TEXTURE3);
            glBindTexture(GL_TEXTURE_2D, currTexture);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, currTexture, 0);
            // Input texture
            glUniform1i(xLoc, 4);
        }

        glBindVertexArray(quadVAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

    }

    // Copy final texture from framebuffer to outputTexture
    glBindTexture(GL_TEXTURE_2D, outputTexture);
    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, gridSize, gridSize);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void diffuse(GLuint texture) {
    glUseProgram(jacobiShaderProgram);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, oceanHeightTexture);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, velocityTexture);

    // Uniform variables
    GLuint alphaLoc = glGetUniformLocation(jacobiShaderProgram, "alpha");
    GLuint rBetaLoc = glGetUniformLocation(jacobiShaderProgram, "rBeta");
    GLuint xLoc = glGetUniformLocation(jacobiShaderProgram, "x");
    GLuint bLoc = glGetUniformLocation(jacobiShaderProgram, "b");
    GLuint gridSizeLoc = glGetUniformLocation(jacobiShaderProgram, "gridSize");
    GLuint scaleLoc = glGetUniformLocation(jacobiShaderProgram, "scale");

    float dx = 1.0 / gridSize;
    float nu = 0.001;
    float alpha = (dx * dx) / (nu * timeStep);

    glUniform1f(alphaLoc, alpha);
    glUniform1f(rBetaLoc, 1.0f / (4.0f + alpha));
    glUniform1f(gridSizeLoc, gridSize);
    glUniform1f(scaleLoc, -1); // velocity

    if (texture == velocityTexture) {
        glUniform1i(bLoc, 1);
        jacobi(velocityTexture, xLoc);
    }

}

void getMouseNDC(GLFWwindow* window, glm::vec2& mouseNDC) {
    // Window coordinates
    double mouseX, mouseY;
    glfwGetCursorPos(window, &mouseX, &mouseY);

    int windowWidth, windowHeight;
    glfwGetWindowSize(window, &windowWidth, &windowHeight);

    mouseNDC.x = (2.0f * static_cast<float>(mouseX) / windowWidth) - 1.0f;
    mouseNDC.y = 1.0f - (2.0f * static_cast<float>(mouseY) / windowHeight);
    std::cout << "Mouse: " << mouseNDC.x << ", " << mouseNDC.y  << std::endl;
}


glm::vec3 computePlaneIntersection(const glm::vec2& mouseNDC) {
    glm::mat4 invVP = glm::inverse(projection * view);

    // Create ray in NDC space (clip space)
    glm::vec4 nearPoint = invVP * glm::vec4(mouseNDC, -1.0f, 1.0f);
    glm::vec4 farPoint = invVP * glm::vec4(mouseNDC, 1.0f, 1.0f);

    // Convert to world coordinates (perspective divide)
    nearPoint /= nearPoint.w;
    farPoint /= farPoint.w;

    glm::vec3 rayOrigin = glm::vec3(nearPoint);
    glm::vec3 rayDirection = glm::normalize(glm::vec3(farPoint) - rayOrigin);

    // Debug: Print ray data
//    std::cout << "Ray Origin: " << rayOrigin.x << ", " << rayOrigin.y << ", " << rayOrigin.z << std::endl;
//    std::cout << "Ray Direction: " << rayDirection.x << ", " << rayDirection.y << ", " << rayDirection.z << std::endl;

    // Check if ray is parallel to the plane
    if (glm::abs(rayDirection.y) < 1e-6f) {
//        std::cout << "Ray is parallel to the plane!" << std::endl;
        return glm::vec3(-1, -1, -1);
    }

    // Compute intersection t for the plane at y = yPlaneHeight
    float t = (yPlaneHeight - rayOrigin.y) / rayDirection.y;

    // If t < 0, intersection is behind the camera
    if (t < 0) {
//        std::cout << "Intersection is behind the camera!" << std::endl;
        return glm::vec3(-1, -1, -1);
    }

    // Compute intersection point
    glm::vec3 intersection = rayOrigin + t * rayDirection;

    // Debug: Print intersection data
//    std::cout << "Intersection: " << intersection.x << ", " << intersection.y << ", " << intersection.z << std::endl;

    // Check if the intersection is within the bounded region
    float halfSize = size / 2.0f;
    if (intersection.x < -halfSize || intersection.x > halfSize ||
        intersection.z < -halfSize || intersection.z > halfSize) {
        std::cout << "Intersection is out of bounds!" << std::endl;
        return glm::vec3(-1, -1, -1); // Outside the boundary
    }

    return intersection;
}



void applyForce(GLFWwindow *window) {
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);

    glm::vec2 mouseNDC;
    getMouseNDC(window, mouseNDC);

    glm::vec3 intersection = computePlaneIntersection(mouseNDC);

    if (intersection == glm::vec3(-1, -1, -1)) return;

    intersection = (intersection + size / 2) / size;

//    std::cout << "Intersection: " << intersection.x << ", " << intersection.y << ", " << intersection.z << std::endl;

    glUseProgram(applyForceShaderProgram);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, velocityTexture);
//    glActiveTexture(GL_TEXTURE6);
//    glBindTexture(GL_TEXTURE_2D, prevHeightTexture);

    GLuint forcePosLoc = glGetUniformLocation(applyForceShaderProgram, "forcePos");
    GLuint forceDirLoc = glGetUniformLocation(applyForceShaderProgram, "forceDir");
    GLuint forceRadiusLoc = glGetUniformLocation(applyForceShaderProgram, "forceRadius");
    GLuint forceStrengthLoc = glGetUniformLocation(applyForceShaderProgram, "forceStrength");
    GLuint velocityTextureLoc = glGetUniformLocation(applyForceShaderProgram, "velocityTexture");
    GLuint gridSizeLoc = glGetUniformLocation(applyForceShaderProgram, "gridSize");
    GLuint sizeLoc = glGetUniformLocation(applyForceShaderProgram, "size");

    glUniform3fv(forcePosLoc, 1, glm::value_ptr(intersection));
    glUniform2f(forceDirLoc, 1.0f, 0.0f);
    glUniform1f(forceRadiusLoc, 0.01);
    glUniform1f(forceStrengthLoc, 5.0f);
    glUniform1i(velocityTextureLoc, 1);
    glUniform1i(gridSizeLoc, gridSize);
    glUniform1f(sizeLoc, size);

    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, velocityTexture, 0);

    glViewport(0, 0, gridSize, gridSize);

    glBindVertexArray(quadVAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

//    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
//    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, prevHeightTexture, 0);
//
//    glViewport(0, 0, gridSize, gridSize);
//
//    glBindVertexArray(quadVAO);
//    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
//    glBindVertexArray(0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

}

void divergence(GLuint divergenceTexture) {
    glUseProgram(divergenceShaderProgram);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, velocityTexture);

    GLuint wLoc = glGetUniformLocation(divergenceShaderProgram, "w");
    GLuint halfrdxLoc = glGetUniformLocation(divergenceShaderProgram, "halfrdx");
    GLuint gridSizeLoc = glGetUniformLocation(divergenceShaderProgram, "gridSize");

    glUniform1i(wLoc, 1);
    glUniform1f(halfrdxLoc, 1.0 / (2.0  * gridSize));
    glUniform1i(gridSizeLoc, gridSize);

    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, divergenceTexture, 0);

    glBindVertexArray(quadVAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void subtractGradient() {
    GLuint outputTexture;
    glGenTextures(1, &outputTexture);
    glBindTexture(GL_TEXTURE_2D, outputTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, gridSize, gridSize, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glUseProgram(gradientSubtractShaderProgram);

    GLuint pLoc = glGetUniformLocation(gradientSubtractShaderProgram, "p");
    GLuint wLoc = glGetUniformLocation(gradientSubtractShaderProgram, "w");
    GLuint halfrdxLoc = glGetUniformLocation(gradientSubtractShaderProgram, "halfrdx");
    GLuint gridSizeLoc = glGetUniformLocation(gradientSubtractShaderProgram, "gridSize");

    glUniform1i(pLoc, 2);
    glUniform1i(wLoc, 1);
    glUniform1f(halfrdxLoc, 1.0 / (2.0  * gridSize));
    glUniform1i(gridSizeLoc, gridSize);

    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, outputTexture, 0);

    glBindVertexArray(quadVAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    glBindTexture(GL_TEXTURE_2D, velocityTexture);
    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, gridSize, gridSize);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glDeleteTextures(1, &outputTexture);
}

void project() {
    // Divergence of intermediate velocity field w
    GLuint divergenceTexture;
    glGenTextures(1, &divergenceTexture);
    glBindTexture(GL_TEXTURE_2D, divergenceTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, gridSize, gridSize, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, divergenceTexture);

    divergence(divergenceTexture);

    glUseProgram(jacobiShaderProgram);

    GLuint alphaLoc = glGetUniformLocation(jacobiShaderProgram, "alpha");
    GLuint rBetaLoc = glGetUniformLocation(jacobiShaderProgram, "rBeta");
    GLuint xLoc = glGetUniformLocation(jacobiShaderProgram, "x");
    GLuint bLoc = glGetUniformLocation(jacobiShaderProgram, "b");
    GLuint gridSizeLoc = glGetUniformLocation(jacobiShaderProgram, "gridSize");
    GLuint scaleLoc = glGetUniformLocation(jacobiShaderProgram, "scale");

    float dx = gridSize;
    float alpha = -(dx * dx);
    float rBeta = 1.0 /4.0;

    glUniform1f(alphaLoc, alpha);
    glUniform1f(rBetaLoc, rBeta);
    glUniform1i(bLoc, 5); // divergence of w
    glUniform1f(gridSizeLoc, gridSize);
    glUniform1f(scaleLoc, 1); //pressure

    // Solve for pressure field
    jacobi(pressureTexture, xLoc);

    subtractGradient();

}

void advectHeight() {
//    GLuint outputTexture;
//    glGenTextures(1, &outputTexture);
//    glBindTexture(GL_TEXTURE_2D, outputTexture);
//    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, gridSize, gridSize, 0, GL_RGBA, GL_FLOAT, nullptr);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);


    glUseProgram(advectHeightShaderProgram);

//        glActiveTexture(GL_TEXTURE5);
//    glBindTexture(GL_TEXTURE_2D, outputTexture);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, oceanHeightTexture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, velocityTexture);


    GLuint velocityTextureLoc = glGetUniformLocation(advectHeightShaderProgram, "velocityTexture");
    GLuint heightFieldLoc = glGetUniformLocation(advectHeightShaderProgram, "heightField");
    GLuint halfrdxLoc = glGetUniformLocation(advectHeightShaderProgram, "halfrdx");
    GLuint gridSizeLoc = glGetUniformLocation(advectHeightShaderProgram, "gridSize");
    GLuint timeStepLoc = glGetUniformLocation(advectHeightShaderProgram, "timeStep");

    glUniform1i(velocityTextureLoc, 1);
    glUniform1i(heightFieldLoc, 0);
    glUniform1f(halfrdxLoc, 1.0 / (2.0  * gridSize));
    glUniform1i(gridSizeLoc, gridSize);
    glUniform1f(timeStepLoc, timeStep);

    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, oceanHeightTexture, 0);

    glViewport(0, 0, gridSize, gridSize);

    glBindVertexArray(quadVAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

//    GLuint srcFBO, dstFBO;
//    glGenFramebuffers(1, &srcFBO);
//    glBindFramebuffer(GL_FRAMEBUFFER, srcFBO);
//    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, outputTexture, 0);
//
//    glGenFramebuffers(1, &dstFBO);
//    glBindFramebuffer(GL_FRAMEBUFFER, dstFBO);
//    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, oceanHeightTexture, 0);
//
//    glBindFramebuffer(GL_READ_FRAMEBUFFER, srcFBO);
//    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dstFBO);
//    glBlitFramebuffer(
//            0, 0, gridSize, gridSize,  // Source rectangle
//            0, 0, gridSize, gridSize,  // Destination rectangle
//            GL_COLOR_BUFFER_BIT,  // What to copy
//            GL_LINEAR        // Filtering mode (GL_NEAREST or GL_LINEAR)
//    );

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
//    glDeleteTextures(1, &outputTexture);
}

void applyGravity() {
    glUseProgram(applyGravityShaderProgram);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, oceanHeightTexture);

    GLuint heightFieldLoc = glGetUniformLocation(applyGravityShaderProgram, "heightField");
    GLuint velocityFieldLoc = glGetUniformLocation(applyGravityShaderProgram, "velocityField");
    GLuint gridSizeLoc = glGetUniformLocation(applyGravityShaderProgram, "gridSize");
    GLuint halfrdxLoc = glGetUniformLocation(applyGravityShaderProgram, "halfrdx");
    GLuint timeStepLoc = glGetUniformLocation(applyGravityShaderProgram, "timeStep");
    GLuint isHeightLoc = glGetUniformLocation(applyGravityShaderProgram, "isHeight");

    glUniform1i(heightFieldLoc, 0);
    glUniform1i(velocityFieldLoc, 1);
    glUniform1i(gridSizeLoc, gridSize);
    glUniform1f(halfrdxLoc, 1.0 / (2.0  * gridSize));
    glUniform1f(timeStepLoc, timeStep);
    glUniform1i(isHeightLoc, 1);

    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, oceanHeightTexture, 0);

    glViewport(0, 0, gridSize, gridSize);

    glBindVertexArray(quadVAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, velocityTexture);

    glUniform1i(isHeightLoc, 0);


    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, velocityTexture, 0);

    glViewport(0, 0, gridSize, gridSize);

    glBindVertexArray(quadVAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);


    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void propagateWave() {
    glUseProgram(propagateWaveShaderProgram);

    GLuint outputTexture;
    glGenTextures(1, &outputTexture);
    glBindTexture(GL_TEXTURE_2D, outputTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, gridSize, gridSize, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, oceanHeightTexture);
    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, outputTexture);
    glActiveTexture(GL_TEXTURE6);
    glBindTexture(GL_TEXTURE_2D, prevHeightTexture);

    GLuint heightFieldLoc = glGetUniformLocation(propagateWaveShaderProgram, "heightField");
    GLuint prevHeightFieldLoc = glGetUniformLocation(propagateWaveShaderProgram, "prevHeightField");
    GLuint gridSizeLoc = glGetUniformLocation(propagateWaveShaderProgram, "gridSize");
    GLuint halfrdxLoc = glGetUniformLocation(propagateWaveShaderProgram, "halfrdx");
    GLuint timeStepLoc = glGetUniformLocation(propagateWaveShaderProgram, "timeStep");

    glUniform1i(heightFieldLoc, 0);
    glUniform1i(prevHeightFieldLoc, 6);
    glUniform1i(gridSizeLoc, gridSize);
    glUniform1f(halfrdxLoc, 1.0 / (2.0  * gridSize));
    glUniform1f(timeStepLoc, timeStep);


    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, outputTexture, 0);

    glViewport(0, 0, gridSize, gridSize);

    glBindVertexArray(quadVAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

// 2. Copy oceanHeightTexture (current height) to prevHeightTexture
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, oceanHeightTexture, 0);
    glBindTexture(GL_TEXTURE_2D, prevHeightTexture);
    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, gridSize, gridSize);

    // 3. Copy outputTexture (new height) to oceanHeightTexture
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, outputTexture, 0);
    glBindTexture(GL_TEXTURE_2D, oceanHeightTexture);
    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, gridSize, gridSize);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glDeleteTextures(1, &outputTexture);
}

void smoothHeight() {
    glUseProgram(smoothHeightShaderProgram);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, oceanHeightTexture);


    GLuint heightFieldLoc = glGetUniformLocation(smoothHeightShaderProgram, "heightField");
    GLuint gridSizeLoc = glGetUniformLocation(smoothHeightShaderProgram, "gridSize");


    glUniform1i(heightFieldLoc, 0);
    glUniform1i(gridSizeLoc, gridSize);

    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, oceanHeightTexture, 0);

    glViewport(0, 0, gridSize, gridSize);

    glBindVertexArray(quadVAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);


    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void integrateVelocity() {
    glUseProgram(velocityIntegrationShaderProgram);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, oceanHeightTexture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, velocityTexture);


    GLuint velocityTextureLoc = glGetUniformLocation(velocityIntegrationShaderProgram, "velocityTexture");
    GLuint heightFieldLoc = glGetUniformLocation(velocityIntegrationShaderProgram, "heightField");
    GLuint halfrdxLoc = glGetUniformLocation(velocityIntegrationShaderProgram, "halfrdx");
    GLuint timeStepLoc = glGetUniformLocation(velocityIntegrationShaderProgram, "timeStep");
    GLuint gridSizeLoc = glGetUniformLocation(velocityIntegrationShaderProgram, "gridSize");

    glUniform1i(velocityTextureLoc, 1);
    glUniform1i(heightFieldLoc, 0);

    glUniform1f(halfrdxLoc, 1.0 / (2.0 * gridSize));
    glUniform1f(timeStepLoc, timeStep);
    glUniform1i(gridSizeLoc, gridSize);

    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, velocityTexture, 0);

    glViewport(0, 0, gridSize, gridSize);

    glBindVertexArray(quadVAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);


    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void cleanup() {
    // Clean up resources
    glDeleteVertexArrays(1, &waterVAO);
    glDeleteBuffers(1, &waterVBO);
    glDeleteBuffers(1, &waterEBO);
    glDeleteProgram(waterShader);
//    glDeleteProgram(skyboxShader);
}


int main() {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    // OpenGL version and core profile
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);  // Required for macOS

    // Create a window and OpenGL context
    GLFWwindow* window = glfwCreateWindow(800, 600, "3D Plane with Water Movement", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        return -1;
    }

    // Print OpenGL version
    std::cout << "OpenGL version: " << glGetString(GL_VERSION) << std::endl;

    // Load and compile shaders
    waterShader = createShaderProgram("../shader.vert", "../shader.frag");
//    waterShader = createShaderProgram("../fullScreenQuad.vert", "../temp.frag"); // texture
    skyboxShader = createShaderProgram("../skyBox.vert", "../skyBox.frag");
    advectShaderProgram = createShaderProgram("../fullScreenQuad.vert", "../advect.frag");
    jacobiShaderProgram = createShaderProgram("../fullScreenQuad.vert", "../jacobi.frag");
    applyForceShaderProgram = createShaderProgram("../fullScreenQuad.vert", "../applyForce.frag");
    divergenceShaderProgram = createShaderProgram("../fullScreenQuad.vert", "../divergence.frag");
    gradientSubtractShaderProgram = createShaderProgram("../fullScreenQuad.vert", "../subtractGradient.frag");
    advectHeightShaderProgram = createShaderProgram("../fullScreenQuad.vert", "../advectHeight.frag");
    displaceHeightShaderProgram = createShaderProgram("../fullScreenQuad.vert", "../displaceHeight.frag");
    applyGravityShaderProgram = createShaderProgram("../fullScreenQuad.vert", "../applyGravity.frag");
    propagateWaveShaderProgram = createShaderProgram("../fullScreenQuad.vert", "../propagateWave.frag");
    velocityIntegrationShaderProgram = createShaderProgram("../fullScreenQuad.vert", "../velocityIntegration.frag");
    initHeightShaderProgram = createShaderProgram("../fullScreenQuad.vert", "../initHeight.frag");
    smoothHeightShaderProgram = createShaderProgram("../fullScreenQuad.vert", "../smoothHeight.frag");

    // Create quadVAO, quadVBO, quadEBO
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glGenBuffers(1, &quadEBO);

    glBindVertexArray(quadVAO);

    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quadEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quadIndices), quadIndices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

//    std::vector<GLfloat> zeroData(gridSize * gridSize * 4, 0.0f);
//    std::vector<GLfloat> colorData(gridSize * gridSize * 4, 0.0f);
//
//    for (int k = 0; k < gridSize; ++k) {
//        for (int j = 0; j < gridSize; ++j) {
//            for (int i = 0; i < gridSize; ++i) {
//                int index = k * gridSize * gridSize + j * gridSize + i;
//                // You can modify the values here if needed
//                colorData[index * 4 + 0] = 2.0f; // Set R to 1.0f, for example
//                colorData[index * 4 + 1] = 0.0f; // G component
//                colorData[index * 4 + 2] = 0.0f; // B component
//                colorData[index * 4 + 3] = 0.0f; // A component
//            }
//        }
//    }

    // Textures
    glActiveTexture(GL_TEXTURE0);
    glGenTextures(1, &oceanHeightTexture);
    glBindTexture(GL_TEXTURE_2D, oceanHeightTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, gridSize, gridSize, 0, GL_RG, GL_FLOAT, zeroData.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);


    glActiveTexture(GL_TEXTURE1);
    glGenTextures(1, &velocityTexture);
    glBindTexture(GL_TEXTURE_2D, velocityTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, gridSize, gridSize, 0, GL_RG, GL_FLOAT, zeroData.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glActiveTexture(GL_TEXTURE2);
    glGenTextures(1, &pressureTexture);
    glBindTexture(GL_TEXTURE_2D, pressureTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, gridSize, gridSize, 0, GL_RG, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glActiveTexture(GL_TEXTURE3);
    glGenTextures(1, &jacobiTexture1);
    glBindTexture(GL_TEXTURE_2D, jacobiTexture1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, gridSize, gridSize, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glActiveTexture(GL_TEXTURE4);
    glGenTextures(1, &jacobiTexture2);
    glBindTexture(GL_TEXTURE_2D, jacobiTexture2);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, gridSize, gridSize, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glActiveTexture(GL_TEXTURE6);
    glGenTextures(1, &prevHeightTexture);
    glBindTexture(GL_TEXTURE_2D, prevHeightTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, gridSize, gridSize, 0, GL_RGBA, GL_FLOAT, zeroData.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Create framebuffer
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, velocityTexture, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Framebuffer is not complete!" << std::endl;
    } else {
        std::cout << "Framebuffer complete!" << std::endl;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    setUpEnvMap();
    setupSkybox();

    setupWater(); // Create vertices for the water height plane

    while (!glfwWindowShouldClose(window)) {
        // Clear screen and depth buffer
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        camera.Inputs(window);
        view = camera.getViewMatrix();
        projection = camera.getProjMatrix(70.0f, 0.1f, 1000.0f);

        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
            applyForce(window);
        }

        advect(velocityTexture);

        advectHeight();
        smoothHeight();
        integrateVelocity();


//        diffuse(velocityTexture);
//        project();


//        propagateWave();



        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        glViewport(0, 0, width, height);


        glDisable(GL_DEPTH_TEST);
        drawSkybox();
        glEnable(GL_DEPTH_TEST);


        // Enable blending for water
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);  // Disable writing to the depth buffer

        drawWater();

        glDepthMask(GL_TRUE);  // Re-enable depth writing
        glDisable(GL_BLEND);


        // Swap front and back buffers
        glfwSwapBuffers(window);

        // Poll for and process events
        glfwPollEvents();
    }


    cleanup();
    glfwTerminate();

    return 0;
}