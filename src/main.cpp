#include <iostream>

#include <glad/glad.h>
#include <glfw/glfw3.h>

int main(int argc, char** argv) {
	// Check GlFW initialization
	if (!glfwInit())
		return -1;
		
	// Setup OpenGL context
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	
	// Create window
	GLFWwindow * window = glfwCreateWindow(800, 600, "Test", nullptr, nullptr);
	
	// Check if cannot create window
	if (window == nullptr) {
		glfwTerminate();
		return -1;
	}
	
	// Setup window context
	glfwMakeContextCurrent(window);
	
	// Check if cannot load OpenGL procedures
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		glfwTerminate();
		return -1;
	}
	
	// Render loop
	while (!glfwWindowShouldClose(window)) {
		// Setup color buffer
		glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		
		// Swap double buffer
		glfwSwapBuffers(window);
		
		// Process events and callbacks
		glfwPollEvents();
	}
	
	// Deinitialize GLFW
	glfwTerminate();
	
	return 0;
}
