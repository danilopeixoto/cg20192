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
glm::mat4 VIEW(1.0f);
glm::mat4 MODEL(1.0f);

struct Light {
    glm::vec3 position;
    glm::vec3 color;
} LIGHT;

struct Material {
    glm::vec3 color;
} MATERIAL;

// Read 8-bit RGB image from Netpbm binary file format (PPM)
// and convert to 32-bit linear RGB image
bool readImage(
        const std::string & filename,
        size_t & width, size_t & height,
        std::vector<glm::vec3> & pixels) {
    std::ifstream file(filename, std::ifstream::in | std::ifstream::binary);
    
    if (!file.is_open())
        return false;
    
    std::string magic, comment;
    size_t w, h, depth;
    
    file >> magic;
    
    file.ignore();

    if (file.peek() == '#')
        std::getline(file, comment);
    
    file >> w >> h >> depth;
    
    file.ignore();
    
    if (magic != "P6" || depth == 0) {
        file.close();
        return false;
    }
    
    float inverseDepth = 1.0f / depth;
    size_t size = w * h;
    
    width = w;
    height = h;
    
    pixels.resize(size);
    
    for (size_t i = 0; i < size; i++) {
        glm::vec3 & pixel = pixels[i];
        unsigned char data[3];
        
        file.read((char *)data, sizeof(data));
        
        pixel.r = glm::clamp((float)data[0] * inverseDepth, 0.0f, 1.0f);
        pixel.g = glm::clamp((float)data[1] * inverseDepth, 0.0f, 1.0f);
        pixel.b = glm::clamp((float)data[2] * inverseDepth, 0.0f, 1.0f);
    }
    
    file.close();
    
    return true;
}

// Load 32-bit linear RBG image to OpenGL
GLuint loadImage(size_t width, size_t height, const std::vector<glm::vec3> & pixels) {
    GLuint textureID;
    
    // Create and bind texture
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    
    // Setup texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    
    // Copy pixel data to texture
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, width, height, 0, GL_RGB, GL_FLOAT, pixels.data());
    
    // Generate mipmap textures
    glGenerateMipmap(GL_TEXTURE_2D);
    
    return textureID;
}

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
    // Duplicate and expand the triangle vertices in a flat array
    struct Vertex {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec2 textureCoordinate;
    };
    
    bool hasNormals = normalIndices.size() > 0;
    bool hasTextureCoordinates = textureCoordinateIndices.size() > 0;
    
    size_t vertexCount = positionIndices.size();
    size_t vertexSize = sizeof(Vertex);
    
    std::vector<Vertex> vertices;
    vertices.reserve(vertexCount);
    
    for (size_t i = 0; i < vertexCount / 3; i++) {
        Vertex triangleVertices[3];
        
        for (size_t j = 0; j < 3; j++) {
            Vertex & vertex = triangleVertices[j];
            vertex.position = positions[positionIndices[i * 3 + j]];
        }
        
        if (hasNormals) {
            for (size_t j = 0; j < 3; j++){
                Vertex & vertex = triangleVertices[j];
                vertex.normal = normals[normalIndices[i * 3 + j]];
            }
        }
        else {
            Vertex & vertex0 = triangleVertices[0];
            Vertex & vertex1 = triangleVertices[1];
            Vertex & vertex2 = triangleVertices[2];
            
            glm::vec3 u = vertex1.position - vertex0.position;
            glm::vec3 v = vertex2.position - vertex0.position;
            
            glm::vec3 n = glm::normalize(glm::cross(u, v));
            
            for (size_t j = 0; j < 3; j++) {
                Vertex & vertex = triangleVertices[j];
                vertex.normal = n;
            }
        }
        
        for (size_t j = 0; j < 3; j++) {
            Vertex & vertex = triangleVertices[j];
            
            if (hasTextureCoordinates)
                vertex.textureCoordinate = textureCoordinates[textureCoordinateIndices[i * 3 + j]];
            else
                vertex.textureCoordinate = glm::vec2((float)(j == 1), (float)(j == 2));
        }
        
        for (size_t j = 0; j < 3; j++)
            vertices.push_back(triangleVertices[j]);
    }
    
    // Create and bind vertex array object
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    
    // Create and bind vertex buffer object
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    
    // Copy vertex attribute data to vertex buffer object
    glBufferData(
        GL_ARRAY_BUFFER,
        vertices.size() * vertexSize,
        vertices.data(),
        usage);
    
    // Define position attribute to shader program
    glVertexAttribPointer(
        0,
        3,
        GL_FLOAT,
        GL_FALSE,
        vertexSize,
        (const GLvoid *)nullptr);
    
    // Enable position attribute to shader program
    glEnableVertexAttribArray(0);
    
    // Define normal attribute to shader program
    glVertexAttribPointer(
        1,
        3,
        GL_FLOAT,
        GL_FALSE,
        vertexSize,
        (const GLvoid *)(3 * sizeof(GLfloat)));
    
    // Enable normal attribute to shader program
    glEnableVertexAttribArray(1);
    
    // Define texture coordinate attribute to shader program
    glVertexAttribPointer(
        2,
        2,
        GL_FLOAT,
        GL_FALSE,
        vertexSize,
        (const GLvoid *)(6 * sizeof(GLfloat)));
    
    // Enable texture coordinate attribute to shader program
    glEnableVertexAttribArray(2);
    
    // Return vertex count or three times the triangle count
    return vertexCount;
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
    
    if (height > 0)
        PROJECTION = glm::perspective(45.0f, width / (float)height, 0.001f, 1000.0f);
}

// Keyboard event callback
void keyboard(
        GLFWwindow * window,
        int key, int scancode, int action, int modifier) {
    if (key == GLFW_KEY_A && action == GLFW_PRESS)
        BACKGROUND_STATE = !BACKGROUND_STATE;
    
    if (key == GLFW_KEY_LEFT && (action == GLFW_PRESS || action == GLFW_REPEAT))
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
    if (!createProgram("../res/shaders/diffuse", programID)) {
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
    
    if (!readTriangleMesh(
            "../res/meshes/bunny_unwrapped.obj",
            positions,
            normals,
            textureCoordinates,
            positionIndices,
            normalIndices,
            textureCoordinateIndices)) {
        glfwTerminate();

        std::cout << "Cannot read triangle mesh." << std::endl;
        return -1;
    };
    
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
    
    // Read 8-bit RGB texture from Netpbm binary file format (PPM)
    size_t width, height;
    std::vector<glm::vec3> pixels;
    
    if (!readImage("../res/textures/checkboard.ppm", width, height, pixels)) {
        glfwTerminate();
        
        std::cout << "Cannot read image." << std::endl;
        return -1;
    };
    
    // Load 32-bit linear RGB texture to OpenGL
    GLuint textureID = loadImage(width, height, pixels);
    
    // Setup view matrix
    VIEW = glm::lookAt(
        glm::vec3(6.0f, 3.0f, 6.0f),
        glm::vec3(0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f));
    
    // Initialize projection matrix and viewport
    resize(window, 800, 600);
    
    // Initialize light parameters
    LIGHT.position = glm::vec3(0.0f, 10.0f, 0.0f);
    LIGHT.color = glm::vec3(1.0f, 1.0f, 1.0f) * 200.0f;
    
    MATERIAL.color = glm::vec3(1.0f, 1.0f, 1.0f);
    
    // Get model matrix location in shader program
    GLint modelLocationID = glGetUniformLocation(programID, "model");
    
    // Get view matrix location in shader program
    GLint viewLocationID = glGetUniformLocation(programID, "view");
    
    // Get projection matrix location in shader program
    GLint projectionLocationID = glGetUniformLocation(programID, "projection");
    
    // Get light position location in shader program
    GLint lightPositionLocationID = glGetUniformLocation(programID, "light.position");
    
    // Get light color location in shader program
    GLint lightColorLocationID = glGetUniformLocation(programID, "light.color");
    
    // Get material color location in shader program
    GLint materialColorLocationID = glGetUniformLocation(programID, "material.color");
    
    // Get image location in shader program
    GLint imageLocationID = glGetUniformLocation(programID, "image");
    
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
        
        // Load model matrix as parameter to shader program
        glUniformMatrix4fv(modelLocationID, 1, GL_FALSE, glm::value_ptr(MODEL));
        
        // Load view matrix as parameter to shader program
        glUniformMatrix4fv(viewLocationID, 1, GL_FALSE, glm::value_ptr(VIEW));
        
        // Load projection matrix as parameter to shader program
        glUniformMatrix4fv(projectionLocationID, 1, GL_FALSE, glm::value_ptr(PROJECTION));
        
        // Load light position as parameter to shader program
        glUniform3fv(lightPositionLocationID, 1, glm::value_ptr(LIGHT.position));
        
        // Load light color as parameter to shader program
        glUniform3fv(lightColorLocationID, 1, glm::value_ptr(LIGHT.color));
        
        // Load material color as parameter to shader program
        glUniform3fv(materialColorLocationID, 1, glm::value_ptr(MATERIAL.color));
        
        // Bind texture to texture unit
        glActiveTexture(GL_TEXTURE0);
        
        // Load texture unit as sampler parameter to shader program
        glUniform1i(imageLocationID, 0);
        
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
    
    // Delete texture
    glDeleteTextures(1, &textureID);

    // Destroy window
    glfwDestroyWindow(window);

    // Deinitialize GLFW
    glfwTerminate();

    return 0;
}
