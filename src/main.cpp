#include <glad/glad.h>
#include <glfw/glfw3.h>

#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include <glm/vec3.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>

#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>

// Global variables
bool BACKGROUND_STATE = false;

glm::mat4 PROJECTION(1.0f);
glm::mat4 MODEL(1.0f);

// Read triangle mesh from Wavefront OBJ file format
bool readTriangleMesh(
        const std::string & filename,
        std::vector<glm::vec3> & positions,
        std::vector<glm::vec3> & normals,
        std::vector<glm::vec2> & textureCoordinates,
        std::vector<size_t> & positionIndices,
        std::vector<size_t> & normalIndices,
        std::vector<size_t> & textureCoordinateIndices) {
    std::ifstream file(filename, std::ifstream::in);

    if (!file.is_open())
        return false;

    std::string line;

    while (std::getline(file, line)) {
        std::istringstream attributes(line);

        std::string type;
        attributes >> type;

        if (type == "v") {
            glm::vec3 position;
            attributes >> position.x >> position.y >> position.z;

            positions.push_back(position);
        }
        else if (type == "vt") {
            glm::vec2 textureCoordinate;
            attributes >> textureCoordinate.x >> textureCoordinate.y;

            textureCoordinates.push_back(textureCoordinate);
        }
        else if (type == "vn") {
            glm::vec3 normal;
            attributes >> normal.x >> normal.y >> normal.z;

            normals.push_back(normal);
        }
        else if (type == "f") {
            for (size_t i = 0; i < 3; i++) {
                std::string tokens;
                attributes >> tokens;

                std::replace(tokens.begin(), tokens.end(), '/', ' ');

                std::istringstream indices(tokens);
                size_t index;

                indices >> index;
                positionIndices.push_back(index - 1);

                if (indices.peek() == ' ') {
                    indices.ignore();

                    if (indices.peek() == ' ') {
                        indices.ignore();

                        indices >> index;
                        normalIndices.push_back(index - 1);
                    }
                    else {
                        indices >> index;
                        textureCoordinateIndices.push_back(index - 1);

                        if (indices.peek() == ' ') {
                            indices.ignore();

                            indices >> index;
                            normalIndices.push_back(index - 1);
                        }
                    }
                }
            }
        }
    }

    file.close();

    return true;
}

// EXERCÍCIO
//
// IMPLEMENTAR A FUNÇÃO ABAIXO VALENDO 10 PONTOS EXTRAS.
// ATUALMENTE ELA NÃO UTILIZA OS PARÂMETROS DE ENTRADA E
// CARREGA UM TRIÂNGULO CONSTANTE COM ATRIBUTOS DE POSIÇÃO E COR.
// O OBJETIVO É MODIFICÁ-LA PARA CARREGAR OS TRIÂNGULOS DESCRITOS
// PELOS PARÂMETROS, SEGUINDO A SUA DOCUMENTAÇÃO:
//
// Load triangle mesh to OpenGL
// Normal and texture coordinate attributes are calculated by primitive when not available
// Vertex attributes are exported to shader program at locations:
// 0: position
// 1: normal
// 2: texture coordinate
size_t loadTriangleMesh(
        const std::vector<glm::vec3> & positions,
        const std::vector<glm::vec3> & normals,
        const std::vector<glm::vec2> & textureCoordinates,
        const std::vector<size_t> & positionIndices,
        const std::vector<size_t> & normalIndices,
        const std::vector<size_t> & textureCoordinateIndices,
        GLenum usage,
        GLuint & vao,
        GLuint & vbo) {
    // Triangle description
    GLfloat vertices[] = {
        -0.5f, -0.5f, 0.0f, // First vertex position
         1.0f,  0.0f, 0.0f, // First vertex color
         
         0.5f, -0.5f, 0.0f, // Second vertex position
         0.0f,  1.0f, 0.0f, // Second vertex color
         
         0.0f,  0.5f, 0.0f, // Third vertex position
         0.0f,  0.0f, 1.0f, // Third vertex color
    };
    
    // Create and bind vertex array object
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    
    // Create and bind vertex buffer object
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    
    // Copy vertex attribute data to vertex buffer object
    glBufferData(
        GL_ARRAY_BUFFER,
        sizeof(vertices),
        vertices,
        usage);
    
    // Define position attribute to shader program
    glVertexAttribPointer(
        0,
        3,
        GL_FLOAT,
        false,
        6 * sizeof(GLfloat),
        (const GLvoid *)nullptr);
    
    // Enable position attribute to shader program
    glEnableVertexAttribArray(0);
    
    // Define color attribute to shader program
    glVertexAttribPointer(
        1,
        3,
        GL_FLOAT,
        false,
        6 * sizeof(GLfloat),
        (const GLvoid *)(3 * sizeof(GLfloat)));
    
    // Enable color attribute to shader program
    glEnableVertexAttribArray(1);
    
    // Return vertex count or three times the triangle count
    return 3;
}

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

// Resize event callback
void resize(GLFWwindow * window, int width, int height) {
    glViewport(0, 0, width, height);
    
    PROJECTION = glm::perspective(45.0f, width / (float)height, 0.001f, 1000.0f);
}

// Keyboard event callback
void keyboard(
        GLFWwindow * window,
        int key, int scancode, int action, int modifier) {
    if (key == GLFW_KEY_A && action == GLFW_PRESS)
        BACKGROUND_STATE = !BACKGROUND_STATE;
    
    if (key == GLFW_KEY_RIGHT && (action == GLFW_PRESS || action == GLFW_REPEAT))
        MODEL = glm::rotate(MODEL, 0.1f, glm::vec3(0.0f, 1.0f, 0.0f));
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

    // Access vertex position attributes of the first triangle
    //
    // std::vector<glm::vec3> positions;
    // std::vector<glm::vec3> normals;
    // std::vector<glm::vec2> textureCoordinates;
    // std::vector<size_t> positionIndices;
    // std::vector<size_t> normalIndices;
    // std::vector<size_t> textureCoordinateIndices;

    // if (loadTriangleMesh(
    //         "../res/meshes/bunny.obj",
    //         positions, normals, textureCoordinates,
    //         positionIndices, normalIndices, textureCoordinateIndices)) {
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
    glfwWindowHint(GLFW_SAMPLES, 16);

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
    
    // Shader program ID
    GLuint programID;
    
    // Check if cannot create shader program
    if (!createProgram("../res/shaders/triangle", programID)) {
        glfwTerminate();

        std::cout << "Cannot create shader program." << std::endl;
        return -1;
    }
    
    // Use shader program
    glUseProgram(programID);
    
    // Enable depth test
    glEnable(GL_DEPTH_TEST);
    
    // Read triangle mesh from Wavefront OBJ file format
    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> textureCoordinates;
    std::vector<size_t> positionIndices;
    std::vector<size_t> normalIndices;
    std::vector<size_t> textureCoordinateIndices;
    
    readTriangleMesh(
        "../res/meshes/bunny.obj",
        positions,
        normals,
        textureCoordinates,
        positionIndices,
        normalIndices,
        textureCoordinateIndices);
    
    // Load triangle mesh to OpenGL
    GLuint vao, vbo;
    
    size_t vertexCount = loadTriangleMesh(
        positions,
        normals,
        textureCoordinates,
        positionIndices,
        normalIndices,
        textureCoordinateIndices,
        GL_STATIC_DRAW,
        vao,
        vbo);
    
    // Setup view matrix
    glm::mat4 view = glm::lookAt(
        glm::vec3(0.0f, 0.0f, 10.0f),
        glm::vec3(0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f));
    
    // Initialize projection matrix and viewport
    resize(window, 800, 600);
    
    // Render loop
    while (!glfwWindowShouldClose(window)) {
        // Setup color buffer
        if (BACKGROUND_STATE)
            glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
        else
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            
        // Clear color buffer
        glClear(GL_COLOR_BUFFER_BIT);
        
        // Setup depth buffer
        glClearDepth(1.0f);
        
        // Clear depth buffer
        glClear(GL_DEPTH_BUFFER_BIT);
        
        // Pass model matrix as parameter to shader program
        GLint modelLocationID = glGetUniformLocation(programID, "model");
        glUniformMatrix4fv(modelLocationID, 1, GL_FALSE, glm::value_ptr(MODEL));
        
        // Pass view matrix as parameter to shader program
        GLint viewLocationID = glGetUniformLocation(programID, "view");
        glUniformMatrix4fv(viewLocationID, 1, GL_FALSE, glm::value_ptr(view));
        
        // Pass projection matrix as parameter to shader program
        GLint projectionLocationID = glGetUniformLocation(programID, "projection");
        glUniformMatrix4fv(projectionLocationID, 1, GL_FALSE, glm::value_ptr(PROJECTION));
        
        // Draw vertex array as triangles
        glDrawArrays(GL_TRIANGLES, 0, vertexCount);
        
        // Swap double buffer
        glfwSwapBuffers(window);

        // Process events and callbacks
        glfwPollEvents();
    }

    // Delete shader program
    glDeleteProgram(programID);

    // Delete vertex array object
    glDeleteVertexArrays(1, &vao);

    // Delete vertex buffer object
    glDeleteBuffers(1, &vbo);

    // Destroy window
    glfwDestroyWindow(window);

    // Deinitialize GLFW
    glfwTerminate();

    return 0;
}
