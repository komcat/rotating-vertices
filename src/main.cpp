#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <openglDebug.h>
#include <demoShaderLoader.h>
#include "VerticesLoader.h"
#include "CameraController.h"
#include "InputHandler.h"
#include <iostream>
#include <vector>
#include <array>
#include <algorithm>
#include <limits>

#define USE_GPU_ENGINE 0
extern "C"
{
  __declspec(dllexport) unsigned long NvOptimusEnablement = USE_GPU_ENGINE;
  __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = USE_GPU_ENGINE;
}

// Global camera controller
CameraController camera;

// Global OpenGL buffer IDs for updating scan data
unsigned int g_lineVAO, g_pointVAO, g_boxVAO, g_VBO, g_lineEBO, g_pointEBO, g_boxVBO, g_boxEBO;

// Cached scan data to avoid regenerating every frame
std::vector<float> g_cachedMeasurementValues;
std::pair<float, float> g_cachedValueRange;
std::vector<unsigned int> g_cachedPointIndices;
std::vector<unsigned int> g_cachedLineIndices;
bool g_needsColorUpdate = true;

// Bounding box data
struct BoundingBox {
  float minX, maxX;
  float minY, maxY;
  float minZ, maxZ;
};
BoundingBox g_boundingBox;

// Forward declarations
void updateScanBuffers();
void valueToColor(float normalizedValue, float& r, float& g, float& b);
void updateCachedData();
void calculateBoundingBox();
void setupBoundingBoxBuffers();
void renderBoundingBox(GLint colorLocation);

// Simple matrix utilities that weren't moved to CameraController
Mat4 mat4Identity() {
  Mat4 result = { 0 };
  result[0] = result[5] = result[10] = result[15] = 1.0f;
  return result;
}

// Create orthographic projection matrix
Mat4 createOrthographicMatrix(float width, float height, float near, float far, float zoom) {
  Mat4 result = { 0 };

  float left = -width / (2.0f * zoom);
  float right = width / (2.0f * zoom);
  float bottom = -height / (2.0f * zoom);
  float top = height / (2.0f * zoom);

  result[0] = 2.0f / (right - left);
  result[5] = 2.0f / (top - bottom);
  result[10] = -2.0f / (far - near);
  result[12] = -(right + left) / (right - left);
  result[13] = -(top + bottom) / (top - bottom);
  result[14] = -(far + near) / (far - near);
  result[15] = 1.0f;

  return result;
}

// Create simple view matrix for orthographic view with pan support
Mat4 createOrthographicViewMatrix() {
  // Simple isometric-like view without perspective distortion
  Mat4 result = mat4Identity();

  // Apply rotation to get isometric view
  float cosX = cos(0.615f);  // ~35.26 degrees
  float sinX = sin(0.615f);
  float cosY = cos(0.785f);  // ~45 degrees
  float sinY = sin(0.785f);

  // Rotation around Y axis then X axis for isometric view
  result[0] = cosY;
  result[2] = sinY;
  result[4] = sinX * sinY;
  result[5] = cosX;
  result[6] = -sinX * cosY;
  result[8] = -cosX * sinY;
  result[9] = sinX;
  result[10] = cosX * cosY;

  // Apply pan offset
  auto panOffset = camera.getPanOffset();
  result[12] = panOffset.first;
  result[13] = panOffset.second;

  return result;
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

// Function to calculate bounding box from scan vertices
void calculateBoundingBox() {
  std::vector<float> scanVertices = VerticesLoader::generateScanVertices();

  if (scanVertices.empty()) {
    g_boundingBox = { 0, 0, 0, 0, 0, 0 };
    std::cout << "Warning: No vertices for bounding box calculation!" << std::endl;
    return;
  }

  g_boundingBox.minX = g_boundingBox.maxX = scanVertices[0];
  g_boundingBox.minY = g_boundingBox.maxY = scanVertices[1];
  g_boundingBox.minZ = g_boundingBox.maxZ = scanVertices[2];

  for (size_t i = 0; i < scanVertices.size(); i += 3) {
    float x = scanVertices[i];
    float y = scanVertices[i + 1];
    float z = scanVertices[i + 2];

    g_boundingBox.minX = std::min(g_boundingBox.minX, x);
    g_boundingBox.maxX = std::max(g_boundingBox.maxX, x);
    g_boundingBox.minY = std::min(g_boundingBox.minY, y);
    g_boundingBox.maxY = std::max(g_boundingBox.maxY, y);
    g_boundingBox.minZ = std::min(g_boundingBox.minZ, z);
    g_boundingBox.maxZ = std::max(g_boundingBox.maxZ, z);
  }

  std::cout << "Bounding box calculated - Vertices: " << (scanVertices.size() / 3) << std::endl;
  std::cout << "  X: [" << g_boundingBox.minX << " to " << g_boundingBox.maxX << "]" << std::endl;
  std::cout << "  Y: [" << g_boundingBox.minY << " to " << g_boundingBox.maxY << "]" << std::endl;
  std::cout << "  Z: [" << g_boundingBox.minZ << " to " << g_boundingBox.maxZ << "]" << std::endl;
}

// Setup bounding box vertex and element buffers
void setupBoundingBoxBuffers() {
  // Define the 8 vertices of the bounding box
  std::vector<float> boxVertices = {
    // Bottom face (z = minZ)
    g_boundingBox.minX, g_boundingBox.minY, g_boundingBox.minZ, // 0
    g_boundingBox.maxX, g_boundingBox.minY, g_boundingBox.minZ, // 1
    g_boundingBox.maxX, g_boundingBox.maxY, g_boundingBox.minZ, // 2
    g_boundingBox.minX, g_boundingBox.maxY, g_boundingBox.minZ, // 3
    // Top face (z = maxZ)
    g_boundingBox.minX, g_boundingBox.minY, g_boundingBox.maxZ, // 4
    g_boundingBox.maxX, g_boundingBox.minY, g_boundingBox.maxZ, // 5
    g_boundingBox.maxX, g_boundingBox.maxY, g_boundingBox.maxZ, // 6
    g_boundingBox.minX, g_boundingBox.maxY, g_boundingBox.maxZ  // 7
  };

  // Define box edges (12 edges total)
  std::vector<unsigned int> boxIndices = {
    // Bottom face edges
    0, 1,  1, 2,  2, 3,  3, 0,
    // Top face edges  
    4, 5,  5, 6,  6, 7,  7, 4,
    // Vertical edges
    0, 4,  1, 5,  2, 6,  3, 7
  };

  std::cout << "Setting up bounding box buffers with " << boxVertices.size() / 3 << " vertices and " << boxIndices.size() / 2 << " edges" << std::endl;

  // Create and setup box VAO
  glGenVertexArrays(1, &g_boxVAO);
  glGenBuffers(1, &g_boxVBO);
  glGenBuffers(1, &g_boxEBO);

  glBindVertexArray(g_boxVAO);

  glBindBuffer(GL_ARRAY_BUFFER, g_boxVBO);
  glBufferData(GL_ARRAY_BUFFER, boxVertices.size() * sizeof(float), boxVertices.data(), GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_boxEBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, boxIndices.size() * sizeof(unsigned int), boxIndices.data(), GL_STATIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);

  glBindVertexArray(0);
  std::cout << "Bounding box buffers setup complete" << std::endl;
}

// Render bounding box with solid lines for front faces and dashed lines for back faces
void renderBoundingBox(GLint colorLocation) {
  glBindVertexArray(g_boxVAO);

  // Set box color (white/gray)
  glUniform3f(colorLocation, 0.8f, 0.8f, 0.8f);

  // Render wireframe box
  glLineWidth(1.0f);
  glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, 0); // 24 indices = 12 edges

  glBindVertexArray(0);
}

// Function to update cached scan data
void updateCachedData() {
  g_cachedMeasurementValues = VerticesLoader::getMeasurementValues();
  g_cachedValueRange = VerticesLoader::getValueRange();
  g_cachedPointIndices = VerticesLoader::generateScanPointIndices();
  g_cachedLineIndices = VerticesLoader::generateScanLineIndices();
  g_needsColorUpdate = true;

  std::cout << "Cached data updated:" << std::endl;
  std::cout << "  Points: " << g_cachedPointIndices.size() << std::endl;
  std::cout << "  Lines: " << g_cachedLineIndices.size() / 2 << std::endl;
  std::cout << "  Values: " << g_cachedMeasurementValues.size() << std::endl;
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

  // Recalculate and update bounding box
  calculateBoundingBox();
  setupBoundingBoxBuffers();

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

  GLFWwindow* window = glfwCreateWindow(1024, 768, "Scan Data Visualization - Debug Version", NULL, NULL);
  if (!window)
  {
    glfwTerminate();
    return -1;
  }

  // Initialize and setup input handling
  InputHandler::initialize();
  InputHandler::setupCallbacks(window);

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

  // Load shaders
  Shader shader;
  if (!shader.loadShaderProgramFromFile(RESOURCES_PATH "vertex.vert", RESOURCES_PATH "fragment.frag")) {
    std::cout << "Failed to load shaders. Exiting." << std::endl;
    glfwTerminate();
    return -1;
  }

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

  if (scanVertices.empty()) {
    std::cout << "No vertices generated. Exiting." << std::endl;
    glfwTerminate();
    return -1;
  }

  std::cout << "Generated data:" << std::endl;
  std::cout << "  Vertices: " << scanVertices.size() / 3 << " points" << std::endl;
  std::cout << "  Lines: " << lineIndices.size() / 2 << " segments" << std::endl;
  std::cout << "  Points: " << pointIndices.size() << " indices" << std::endl;

  // Print first few vertices for debugging
  std::cout << "First few vertices:" << std::endl;
  for (size_t i = 0; i < std::min((size_t)15, scanVertices.size()); i += 3) {
    std::cout << "  [" << i / 3 << "] (" << scanVertices[i] << ", " << scanVertices[i + 1] << ", " << scanVertices[i + 2] << ")" << std::endl;
  }

  // Calculate bounding box
  calculateBoundingBox();

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

  // Setup bounding box buffers
  setupBoundingBoxBuffers();

  // Get uniform locations
  shader.bind();
  GLint colorLocation = shader.getUniform("color");
  GLint modelLocation = shader.getUniform("model");
  GLint viewLocation = shader.getUniform("view");
  GLint projectionLocation = shader.getUniform("projection");

  // Setup camera with enhanced settings
  camera.setMinMaxZoom(0.1f, 100.0f);
  camera.setPanSensitivity(2.0f); // Better pan responsiveness
  camera.setRotationSensitivity(1.5f); // Good rotation speed

  // Print control instructions
  InputHandler::printControlInstructions();

  while (!glfwWindowShouldClose(window))
  {
    int width = 0, height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);

    glClearColor(0.1f, 0.1f, 0.1f, 1.0f); // Dark gray background
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    shader.bind();

    // Create orthographic projection and view matrices with pan and rotation support
    Mat4 projection = createOrthographicMatrix(width, height, -100.0f, 100.0f, camera.getZoom());
    Mat4 view = camera.getViewMatrix(); // Use camera's view matrix with rotation support
    Mat4 model = mat4Identity(); // No model transformation needed

    // Set matrices
    glUniformMatrix4fv(projectionLocation, 1, GL_FALSE, projection.data());
    glUniformMatrix4fv(viewLocation, 1, GL_FALSE, view.data());
    glUniformMatrix4fv(modelLocation, 1, GL_FALSE, model.data());

    // Render bounding box first (so it appears behind other elements)
    renderBoundingBox(colorLocation);

    // Render lines in green
    glUniform3f(colorLocation, 0.0f, 1.0f, 0.0f);
    glBindVertexArray(g_lineVAO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_lineEBO);
    if (!g_cachedLineIndices.empty()) {
      glDrawElements(GL_LINES, g_cachedLineIndices.size(), GL_UNSIGNED_INT, 0);
    }

    // Render points - Set point size properly and render as individual points
    glBindVertexArray(g_pointVAO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_pointEBO);

    // Set a reasonable point size
    float pointSize = 5.0f * camera.getZoom();
    pointSize = std::max(1.0f, std::min(pointSize, 20.0f)); // Clamp between 1 and 20

    // Note: Point size needs to be set in vertex shader or use GL_PROGRAM_POINT_SIZE
    glEnable(GL_PROGRAM_POINT_SIZE);

    // Safety check - ensure we have data to render
    if (!g_cachedPointIndices.empty() && !g_cachedMeasurementValues.empty()) {
      // Calculate colors for points
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
        pointColors.reserve(g_cachedMeasurementValues.size() * 3);

        for (size_t i = 0; i < g_cachedMeasurementValues.size(); ++i) {
          float val = g_cachedMeasurementValues[i];
          float r, g, b;

          if (val < 0 || val < MIN_VALID_VALUE) {
            r = g = b = 0.5f; // Gray for invalid values
          }
          else if (maxValidValue == minValidValue) {
            r = 0.0f; g = 1.0f; b = 0.0f; // Green for single value
          }
          else {
            float normalizedValue = (val - minValidValue) / (maxValidValue - minValidValue);
            normalizedValue = std::max(0.0f, std::min(1.0f, normalizedValue));
            valueToColor(normalizedValue, r, g, b);
          }

          pointColors.push_back(r);
          pointColors.push_back(g);
          pointColors.push_back(b);
        }

        colorsCalculated = true;
        g_needsColorUpdate = false;
        std::cout << "Point colors calculated for " << g_cachedMeasurementValues.size() << " points" << std::endl;
      }

      // Draw each point with its individual color
      for (size_t i = 0; i < g_cachedPointIndices.size() && i < pointColors.size() / 3; ++i) {
        float r = pointColors[i * 3];
        float g = pointColors[i * 3 + 1];
        float b = pointColors[i * 3 + 2];
        glUniform3f(colorLocation, r, g, b);

        // Draw single point
        glDrawElements(GL_POINTS, 1, GL_UNSIGNED_INT, (void*)(i * sizeof(unsigned int)));
      }
    }
    else {
      std::cout << "Warning: No point data to render" << std::endl;
    }

    // Unbind everything
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  // Cleanup
  glDeleteVertexArrays(1, &g_lineVAO);
  glDeleteVertexArrays(1, &g_pointVAO);
  glDeleteVertexArrays(1, &g_boxVAO);
  glDeleteBuffers(1, &g_VBO);
  glDeleteBuffers(1, &g_lineEBO);
  glDeleteBuffers(1, &g_pointEBO);
  glDeleteBuffers(1, &g_boxVBO);
  glDeleteBuffers(1, &g_boxEBO);

  VerticesLoader::clear();
  glfwTerminate();
  return 0;
}