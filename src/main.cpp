#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <openglDebug.h>
#include <demoShaderLoader.h>
#include "VerticesLoader.h"
#include "CameraController.h"
#include <iostream>
#include <vector>
#include <array>

#define USE_GPU_ENGINE 0
extern "C"
{
  __declspec(dllexport) unsigned long NvOptimusEnablement = USE_GPU_ENGINE;
  __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = USE_GPU_ENGINE;
}

// Global camera controller
CameraController camera;

// Simple matrix utilities that weren't moved to CameraController
Mat4 mat4Identity() {
  Mat4 result = { 0 };
  result[0] = result[5] = result[10] = result[15] = 1.0f;
  return result;
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    glfwSetWindowShouldClose(window, GLFW_TRUE);

  // Zoom controls using camera controller
  if (key == GLFW_KEY_EQUAL || key == GLFW_KEY_KP_ADD) { // + key
    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
      camera.zoomIn();
    }
  }

  if (key == GLFW_KEY_MINUS || key == GLFW_KEY_KP_SUBTRACT) { // - key
    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
      camera.zoomOut();
    }
  }
}

static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
  if (button == GLFW_MOUSE_BUTTON_RIGHT) {
    if (action == GLFW_PRESS) {
      double xpos, ypos;
      glfwGetCursorPos(window, &xpos, &ypos);
      camera.startRotation(xpos, ypos);
      glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }
    else if (action == GLFW_RELEASE) {
      camera.endRotation();
      glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
  }
}

static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
  if (camera.isRotating()) {
    camera.updateRotation(xpos, ypos);
  }
}

static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
  if (yoffset > 0) {
    camera.zoomIn(0.1f);
  }
  else if (yoffset < 0) {
    camera.zoomOut(0.1f);
  }
}

int main(void)
{
  std::cout << "C++ Version: ";
  if (!glfwInit())
    return -1;

#pragma region report opengl errors to std
  //enable opengl debugging output.
  glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
#pragma endregion

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  GLFWwindow* window = glfwCreateWindow(1024, 768, "Scan Data Visualization - Interactive View", NULL, NULL);
  if (!window)
  {
    glfwTerminate();
    return -1;
  }

  // Set up all the callbacks
  glfwSetKeyCallback(window, key_callback);
  glfwSetMouseButtonCallback(window, mouse_button_callback);
  glfwSetCursorPosCallback(window, cursor_position_callback);
  glfwSetScrollCallback(window, scroll_callback);

  glfwMakeContextCurrent(window);
  gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
  glfwSwapInterval(1);

#pragma region report opengl errors to std
  glEnable(GL_DEBUG_OUTPUT);
  glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
  glDebugMessageCallback(glDebugOutput, 0);
  glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
#pragma endregion

  // Enable depth testing
  glEnable(GL_DEPTH_TEST);

  // Enable point size control (for rendering points)
  glEnable(GL_PROGRAM_POINT_SIZE);

  // Load shaders
  Shader shader;
  shader.loadShaderProgramFromFile(RESOURCES_PATH "vertex.vert", RESOURCES_PATH "fragment.frag");

  // Generate scan data using the VerticesLoader class
  std::vector<float> scanVertices = VerticesLoader::generateScaledScanVertices(1000.0f);
  std::vector<unsigned int> lineIndices = VerticesLoader::generateScanLineIndices();
  std::vector<unsigned int> pointIndices = VerticesLoader::generateScanPointIndices();

  // Create VAO, VBO, and EBOs for the scan data
  unsigned int lineVAO, pointVAO, VBO, lineEBO, pointEBO;

  // Line rendering setup
  glGenVertexArrays(1, &lineVAO);
  glGenBuffers(1, &VBO);
  glGenBuffers(1, &lineEBO);

  glBindVertexArray(lineVAO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, scanVertices.size() * sizeof(float), scanVertices.data(), GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, lineEBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, lineIndices.size() * sizeof(unsigned int), lineIndices.data(), GL_STATIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);

  // Point rendering setup
  glGenVertexArrays(1, &pointVAO);
  glGenBuffers(1, &pointEBO);

  glBindVertexArray(pointVAO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO); // Reuse same VBO
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pointEBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, pointIndices.size() * sizeof(unsigned int), pointIndices.data(), GL_STATIC_DRAW);

  glBindVertexArray(0); // Unbind

  // Get uniform locations
  shader.bind();
  GLint colorLocation = shader.getUniform("color");
  GLint modelLocation = shader.getUniform("model");
  GLint viewLocation = shader.getUniform("view");
  GLint projectionLocation = shader.getUniform("projection");

  // Setup camera
  camera.setMinMaxZoom(0.1f, 10.0f);
  camera.setCameraDistance(3.0f);

  std::cout << "=== Scan Data Visualization Controls ===" << std::endl;
  std::cout << "  Right Click + Drag : Rotate Camera" << std::endl;
  std::cout << "  Mouse Wheel        : Zoom In/Out" << std::endl;
  std::cout << "  + or Numpad+       : Zoom In" << std::endl;
  std::cout << "  - or Numpad-       : Zoom Out" << std::endl;
  std::cout << "  ESC                : Exit" << std::endl;
  std::cout << "Current Zoom: " << camera.getZoom() << "x" << std::endl;
  std::cout << "=======================================" << std::endl;

  while (!glfwWindowShouldClose(window))
  {
    int width = 0, height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    shader.bind();

    // Get matrices from camera controller
    float aspect = (float)width / (float)height;
    Mat4 projection = camera.getProjectionMatrix(aspect);
    Mat4 view = camera.getViewMatrix();
    Mat4 model = mat4Identity(); // No model transformation needed

    // Set matrices
    glUniformMatrix4fv(projectionLocation, 1, GL_FALSE, projection.data());
    glUniformMatrix4fv(viewLocation, 1, GL_FALSE, view.data());
    glUniformMatrix4fv(modelLocation, 1, GL_FALSE, model.data());

    // Render lines in green
    glUniform3f(colorLocation, 0.0f, 1.0f, 0.0f);
    glBindVertexArray(lineVAO);
    glDrawElements(GL_LINES, lineIndices.size(), GL_UNSIGNED_INT, 0);

    // Render points in red for better visibility
    glUniform3f(colorLocation, 1.0f, 0.0f, 0.0f);
    glPointSize(5.0f * camera.getZoom()); // Scale point size with zoom
    glBindVertexArray(pointVAO);
    glDrawElements(GL_POINTS, pointIndices.size(), GL_UNSIGNED_INT, 0);

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  // Cleanup
  glDeleteVertexArrays(1, &lineVAO);
  glDeleteVertexArrays(1, &pointVAO);
  glDeleteBuffers(1, &VBO);
  glDeleteBuffers(1, &lineEBO);
  glDeleteBuffers(1, &pointEBO);

  glfwTerminate();
  return 0;
}