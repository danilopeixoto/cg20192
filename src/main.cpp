#include <glad/glad.h>
#include <glfw/glfw3.h>

#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include <glm/vec3.hpp>
#include <glm/mat3x3.hpp>

#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>

// Global variables
bool BACKGROUND_STATE = false;

// Compile shader source code from text file format
bool compileShader(const std::string & filename, GLenum type, GLuint & id) {
    // Read from text file to string
    std::ifstream file(filename, std::ifstream::in);

    if (!file.is_open())
        return false;

    std::stringstream buffer;
    std::string source;

    buffer << file.rdbuf();
    source = buffer.str();

    // Create shader
    GLuint shaderID = glCreateShader(type);

    // Setup shader source code
    const GLchar * src = source.data();
    glShaderSource(shaderID, 1, &src, nullptr);

    // Compile shader
    glCompileShader(shaderID);

    // Get compilation status
    GLint status;
    glGetShaderiv(shaderID, GL_COMPILE_STATUS, &status);

    // Check compilation errors
    if (status != GL_TRUE) {
        // Get log message size of the compilation process
        GLint size = 0;
        glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &size);

        std::string message;
        message.resize(size);

        // Get log message of the compilation process
        glGetShaderInfoLog(shaderID, size, nullptr, (GLchar *)message.data());

        // Print log message
        std::cout << message << std::endl;

        // Delete shader
        glDeleteShader(shaderID);

        return false;
    }

    // Return shader id
    id = shaderID;

    return true;
}

// Create shader program
bool createProgram(const std::string & name, GLuint & id) {
    GLuint vertexShaderID, fragmentShaderID;

    // Load and compile vertex shader
    if (!compileShader(name + ".vert", GL_VERTEX_SHADER, vertexShaderID))
        return false;

    // Load and compile fragment shader
    if (!compileShader(name + ".frag", GL_FRAGMENT_SHADER, fragmentShaderID))
        return false;

    // Create shader program
    GLuint programID = glCreateProgram();

    // Attach compiled shaders to program
    glAttachShader(programID, vertexShaderID);
    glAttachShader(programID, fragmentShaderID);

    // Link attached shaders to create an executable
    glLinkProgram(programID);

    // Delete compiled shaders
    glDeleteShader(vertexShaderID);
    glDeleteShader(fragmentShaderID);

    // Get linkage status
    GLint status;
    glGetProgramiv(programID, GL_LINK_STATUS, &status);

    // Check linkage errors
    if (status != GL_TRUE) {
        // Get log message size of the linkage process
        GLint size = 0;
        glGetProgramiv(programID, GL_INFO_LOG_LENGTH, &size);

        std::string message;
        message.resize(size);

        // Get log message of the linkage process
        glGetProgramInfoLog(programID, size, nullptr, (GLchar *)message.data());

        // Print log message
        std::cout << message << std::endl;

        // Delete shader program
        glDeleteProgram(programID);

        return false;
    }

    // Return shader program id
    id = programID;

    return true;
}

// Load triangle mesh from Wavefront OBJ file format
bool loadTriangleMesh(
    const std::string & filename,
    std::vector<glm::vec3> & positions,
    std::vector<glm::vec3> & normals,
    std::vector<glm::vec2> & textureCoordinates,
    std::vector<size_t> & positionIndices,
    std::vector<size_t> & normalIndices,
    std::vector<size_t> & textureCoordinateIndices);

// Resize event callback
void resize(GLFWwindow * window, int width, int height) {
    glViewport(0, 0, width, height);
}

// Keyboard event callback
void keyboard(
    GLFWwindow * window,
    int key, int scancode, int action, int modifier) {
    if (key == GLFW_KEY_A && action == GLFW_PRESS)
        BACKGROUND_STATE = !BACKGROUND_STATE;
}

int main(int argc, char ** argv) {
    // GLM usage
    //
    // glm::vec3 u(1.0f, 0, 0);
    // glm::vec3 v(0, 1.0f, 0);
    //
    // glm::vec3 d = glm::dot(u, v);
    //
    // glm::mat3 m(0.0f, 1.0f, 2.0f,  // First column
    //			   3.0f, 4.0f, 5.0f,  // Second column
    //			   6.0f, 7.0f, 8.0f); // Third column
    //
    // Print GLM matrix
    // std::cout << glm::to_string(m) << std::endl;
    //
    // Access the first column as GLM vector
    // std::cout << glm::to_string(m[0]) << std::endl;
    //
    // Access the second element of the first column as float
    // std::cout << m[0][1] << std::endl;

    // Access first triangle vertices
    //
    // std::vector<glm::vec3> positions;
    // std::vector<glm::vec3> normals;
    // std::vector<glm::vec2> textureCoordinates;
    // std::vector<size_t> positionIndices;
    // std::vector<size_t> normalIndices;
    // std::vector<size_t> textureCoordinateIndices;

    // if (loadTriangleMesh(
    //		   "../res/meshes/bunny.obj",
    //		   positions, normals, textureCoordinates,
    //	       positionIndices, normalIndices, textureCoordinateIndices)) {
    //	   size_t triangleIndex = 0;
    //	   
    //	   glm::vec3 p0 = positions[positionIndices[triangleIndex * 3]];
    //	   glm::vec3 p1 = positions[positionIndices[triangleIndex * 3 + 1]];
    //	   glm::vec3 p2 = positions[positionIndices[triangleIndex * 3 + 2]];
    //	   
    //	   std::cout << "First triangle vertices: " << std::endl;
    //	   
    //	   std::cout << glm::to_string(p0) << std::endl;
    //	   std::cout << glm::to_string(p1) << std::endl;
    //     std::cout << glm::to_string(p2) << std::endl;
    // }

    // Check GLFW initialization
    if (!glfwInit()) {
        std::cout << "Cannot initialize GLFW." << std::endl;
        return -1;
    }

    // Setup OpenGL context
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create window
    GLFWwindow * window = glfwCreateWindow(800, 600, "Window", nullptr, nullptr);

    // Check if cannot create window
    if (window == nullptr) {
        glfwTerminate();

        std::cout << "Cannot create window." << std::endl;
        return -1;
    }

    // Register event callbacks
    glfwSetFramebufferSizeCallback(window, resize);
    glfwSetKeyCallback(window, keyboard);

    // Setup window context
    glfwMakeContextCurrent(window);

    // Check if cannot load OpenGL procedures
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        glfwTerminate();

        std::cout << "Cannot load OpenGL procedures." << std::endl;
        return -1;
    }

    // Render loop
    while (!glfwWindowShouldClose(window)) {
        // Setup color buffer
        if (BACKGROUND_STATE)
            glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
        else
            glClearColor(0.0f, 1.0f, 0.0f, 1.0f);

        // Clear color buffer
        glClear(GL_COLOR_BUFFER_BIT);

        // Swap double buffer
        glfwSwapBuffers(window);

        // Process events and callbacks
        glfwPollEvents();
    }

    // Destroy window
    glfwDestroyWindow(window);

    // Deinitialize GLFW
    glfwTerminate();

    return 0;
}
