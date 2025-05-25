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
#include <algorithm>

#define USE_GPU_ENGINE 0
extern "C"
{
  __declspec(dllexport) unsigned long NvOptimusEnablement = USE_GPU_ENGINE;
  __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = USE_GPU_ENGINE;
}

// Global camera controller
CameraController camera;

// Global OpenGL buffer IDs for updating scan data
unsigned int g_lineVAO, g_pointVAO, g_VBO, g_lineEBO, g_pointEBO;

// Cached scan data to avoid regenerating every frame
std::vector<float> g_cachedMeasurementValues;
std::pair<float, float> g_cachedValueRange;
std::vector<unsigned int> g_cachedPointIndices;
std::vector<unsigned int> g_cachedLineIndices;
bool g_needsColorUpdate = true;

// Forward declarations
void updateScanBuffers();
void valueToColor(float normalizedValue, float& r, float& g, float& b);
void updateCachedData();

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

  // Reload scan data with R key
  if (key == GLFW_KEY_R && action == GLFW_PRESS) {
    std::cout << "Reloading scan data..." << std::endl;
    if (VerticesLoader::loadMostRecentScan(1000.0f)) {
      std::cout << "Scan data reloaded successfully!" << std::endl;
      std::cout << VerticesLoader::getScanInfo() << std::endl;

      // Update GPU buffers with new data
      updateScanBuffers();
      std::cout << "Updated 3D visualization!" << std::endl;
    }
    else {
      std::cout << "Failed to reload scan data!" << std::endl;
    }
  }

  // Cycle through scan files with Tab key
  if (key == GLFW_KEY_TAB && action == GLFW_PRESS) {
    std::cout << "Loading next scan file..." << std::endl;
    if (VerticesLoader::loadNextScanFile(1000.0f)) {
      std::cout << "Loaded next scan file!" << std::endl;
      std::cout << VerticesLoader::getScanInfo() << std::endl;

      // Update GPU buffers with new data
      updateScanBuffers();
      std::cout << "Updated 3D visualization!" << std::endl;
    }
    else {
      std::cout << "Failed to load next scan file!" << std::endl;
    }
  }

  // Cycle backwards through scan files with Shift+Tab
  if (key == GLFW_KEY_TAB && action == GLFW_PRESS && (mods & GLFW_MOD_SHIFT)) {
    std::cout << "Loading previous scan file..." << std::endl;
    if (VerticesLoader::loadPreviousScanFile(1000.0f)) {
      std::cout << "Loaded previous scan file!" << std::endl;
      std::cout << VerticesLoader::getScanInfo() << std::endl;

      // Update GPU buffers with new data
      updateScanBuffers();
      std::cout << "Updated 3D visualization!" << std::endl;
    }
    else {
      std::cout << "Failed to load previous scan file!" << std::endl;
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

// Helper function to map value to color (0=blue, 0.5=green, 1=red)
void valueToColor(float normalizedValue, float& r, float& g, float& b) {
  // Clamp value between 0 and 1
  normalizedValue = std::max(0.0f, std::min(1.0f, normalizedValue));

  if (normalizedValue < 0.5f) {
    // Blue to Green (0 to 0.5)
    float t = normalizedValue * 2.0f;
    r = 0.0f;
    g = t;
    b = 1.0f - t;
  }
  else {
    // Green to Red (0.5 to 1)
    float t = (normalizedValue - 0.5f) * 2.0f;
    r = t;
    g = 1.0f - t;
    b = 0.0f;
  }
}

// Function to update cached scan data
void updateCachedData() {
  g_cachedMeasurementValues = VerticesLoader::getMeasurementValues();
  g_cachedValueRange = VerticesLoader::getValueRange();
  g_cachedPointIndices = VerticesLoader::generateScanPointIndices();
  g_cachedLineIndices = VerticesLoader::generateScanLineIndices();
  g_needsColorUpdate = true;
  std::cout << "Cached scan data updated" << std::endl;
}

// Function to update GPU buffers with new scan data
void updateScanBuffers() {
  // Generate new scan data
  std::vector<float> scanVertices = VerticesLoader::generateScanVertices();
  std::vector<unsigned int> lineIndices = VerticesLoader::generateScanLineIndices();
  std::vector<unsigned int> pointIndices = VerticesLoader::generateScanPointIndices();

  if (scanVertices.empty()) {
    std::cerr << "Warning: No vertices to update!" << std::endl;
    return;
  }

  std::cout << "Updating buffers - Vertices: " << scanVertices.size() / 3 << ", Lines: " << lineIndices.size() / 2 << ", Points: " << pointIndices.size() << std::endl;

  // Update vertex buffer
  glBindBuffer(GL_ARRAY_BUFFER, g_VBO);
  glBufferData(GL_ARRAY_BUFFER, scanVertices.size() * sizeof(float), scanVertices.data(), GL_STATIC_DRAW);

  // Update line indices buffer
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_lineEBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, lineIndices.size() * sizeof(unsigned int), lineIndices.data(), GL_STATIC_DRAW);

  // Update point indices buffer
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_pointEBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, pointIndices.size() * sizeof(unsigned int), pointIndices.data(), GL_STATIC_DRAW);

  // Unbind buffers
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

  // Update cached data
  updateCachedData();

  std::cout << "Buffer update complete!" << std::endl;
}

int main(void)
{
  if (!glfwInit())
    return -1;

#pragma region report opengl errors to std
  //enable opengl debugging output.
  glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
#pragma endregion

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  GLFWwindow* window = glfwCreateWindow(1024, 768, "Scan Data Visualization - JSON Loader", NULL, NULL);
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

  // Load scan data from JSON file
  std::cout << "Initializing scan file system..." << std::endl;
  if (!VerticesLoader::initializeScanFiles("logs/scanning", 1000.0f)) {
    std::cout << "Failed to initialize scan files. Exiting." << std::endl;
    glfwTerminate();
    return -1;
  }

  // Initialize cached data for first file
  updateCachedData();

  // Display scan information
  std::cout << "=== Scan Data Information ===" << std::endl;
  std::cout << VerticesLoader::getScanInfo() << std::endl;
  std::cout << "=============================" << std::endl;

  // Generate scan data
  std::vector<float> scanVertices = VerticesLoader::generateScanVertices();
  std::vector<unsigned int> lineIndices = VerticesLoader::generateScanLineIndices();
  std::vector<unsigned int> pointIndices = VerticesLoader::generateScanPointIndices();
  std::vector<float> measurementValues = VerticesLoader::getMeasurementValues();
  auto valueRange = VerticesLoader::getValueRange();

  if (scanVertices.empty()) {
    std::cout << "No vertices generated. Exiting." << std::endl;
    glfwTerminate();
    return -1;
  }

  // Create VAO, VBO, and EBOs for the scan data
  // Line rendering setup
  glGenVertexArrays(1, &g_lineVAO);
  glGenBuffers(1, &g_VBO);
  glGenBuffers(1, &g_lineEBO);

  glBindVertexArray(g_lineVAO);
  glBindBuffer(GL_ARRAY_BUFFER, g_VBO);
  glBufferData(GL_ARRAY_BUFFER, scanVertices.size() * sizeof(float), scanVertices.data(), GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_lineEBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, lineIndices.size() * sizeof(unsigned int), lineIndices.data(), GL_STATIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);

  // Point rendering setup
  glGenVertexArrays(1, &g_pointVAO);
  glGenBuffers(1, &g_pointEBO);

  glBindVertexArray(g_pointVAO);
  glBindBuffer(GL_ARRAY_BUFFER, g_VBO); // Reuse same VBO
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_pointEBO);
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
  std::cout << "  Tab                : Next Scan File" << std::endl;
  std::cout << "  Shift + Tab        : Previous Scan File" << std::endl;
  std::cout << "  R                  : Reload Scan Data" << std::endl;
  std::cout << "  ESC                : Exit" << std::endl;
  auto fileInfo = VerticesLoader::getCurrentFileInfo();
  std::cout << "Available Files: " << fileInfo.second << std::endl;
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
    glBindVertexArray(g_lineVAO);

    // Bind the correct element buffer for lines
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_lineEBO);
    glDrawElements(GL_LINES, g_cachedLineIndices.size(), GL_UNSIGNED_INT, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // Render points with individual colors based on measurement values
    glBindVertexArray(g_pointVAO);
    glPointSize(8.0f * camera.getZoom()); // Larger points for better visibility

    // Safety check - ensure we have data to render
    if (g_cachedPointIndices.empty() || g_cachedMeasurementValues.empty()) {
      std::cout << "Warning: No point data to render" << std::endl;
    }
    else {
      // Only do color analysis once or when data changes
      static std::vector<float> pointColors;
      static bool colorsCalculated = false;

      if (g_needsColorUpdate || !colorsCalculated) {
        std::cout << "Calculating individual point colors..." << std::endl;

        // Define value ranges
        const float MIN_VALID_VALUE = 0.000005f; // 5 micro threshold

        // Filter values to get valid range
        std::vector<float> validValues;
        for (float val : g_cachedMeasurementValues) {
          if (val >= MIN_VALID_VALUE) {
            validValues.push_back(val);
          }
        }

        float minValidValue, maxValidValue;
        if (validValues.empty()) {
          std::cout << "No valid values for individual coloring - using gray" << std::endl;
          minValidValue = maxValidValue = 0.0f;
        }
        else {
          minValidValue = *std::min_element(validValues.begin(), validValues.end());
          maxValidValue = *std::max_element(validValues.begin(), validValues.end());
          std::cout << "Individual color range: " << minValidValue << " to " << maxValidValue << std::endl;
        }

        // Calculate color for each point
        pointColors.clear();
        pointColors.reserve(g_cachedMeasurementValues.size() * 3); // RGB for each point

        for (size_t i = 0; i < g_cachedMeasurementValues.size(); ++i) {
          float val = g_cachedMeasurementValues[i];
          float r, g, b;

          if (val < 0 || val < MIN_VALID_VALUE) {
            // Gray for invalid values
            r = g = b = 0.5f;
          }
          else if (maxValidValue == minValidValue) {
            // Single value - use middle color (green)
            r = 0.0f; g = 1.0f; b = 0.0f;
          }
          else {
            // Normalize to 0-1 range based on valid values
            float normalizedValue = (val - minValidValue) / (maxValidValue - minValidValue);
            normalizedValue = std::max(0.0f, std::min(1.0f, normalizedValue));

            // Blue (0) -> Green (0.5) -> Red (1)
            valueToColor(normalizedValue, r, g, b);
          }

          pointColors.push_back(r);
          pointColors.push_back(g);
          pointColors.push_back(b);
        }

        colorsCalculated = true;
        g_needsColorUpdate = false;
        std::cout << "Individual point colors calculated for " << g_cachedMeasurementValues.size() << " points" << std::endl;
      }

      // Bind the correct element buffer for points
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_pointEBO);

      // Draw each point with its individual color
      for (size_t i = 0; i < g_cachedPointIndices.size() && i < pointColors.size() / 3; ++i) {
        // Set color for this point
        float r = pointColors[i * 3];
        float g = pointColors[i * 3 + 1];
        float b = pointColors[i * 3 + 2];
        glUniform3f(colorLocation, r, g, b);

        // Draw single point
        glDrawElements(GL_POINTS, 1, GL_UNSIGNED_INT, (void*)(i * sizeof(unsigned int)));
      }

      // Unbind element buffer
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  // Cleanup
  glDeleteVertexArrays(1, &g_lineVAO);
  glDeleteVertexArrays(1, &g_pointVAO);
  glDeleteBuffers(1, &g_VBO);
  glDeleteBuffers(1, &g_lineEBO);
  glDeleteBuffers(1, &g_pointEBO);

  VerticesLoader::clear();
  glfwTerminate();
  return 0;
}