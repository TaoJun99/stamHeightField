cmake_minimum_required(VERSION 3.26)
project(stamHeightField)

set(CMAKE_CXX_STANDARD 17)

file(GLOB SOURCE_FILES  main.cpp shader.frag shader.vert)

add_executable(stamHeightField main.cpp
        Camera.cpp
        Camera.h)

find_package(glfw3 REQUIRED)
target_link_libraries(stamHeightField PRIVATE glfw)

find_package(GLEW REQUIRED)
target_link_libraries(stamHeightField PRIVATE GLEW::GLEW)

find_package(glm REQUIRED)
target_include_directories(stamHeightField PRIVATE ${GLM_INCLUDE_DIRS})

# Link OpenGL
target_link_libraries(stamHeightField PRIVATE ${OPENGL_gl_LIBRARY})

target_include_directories(stamHeightField PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_directories(stamHeightField PRIVATE ${CMAKE_SOURCE_DIR}/include)
