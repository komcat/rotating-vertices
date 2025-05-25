#include "InputHandler.h"
#include "CameraController.h"
#include "VerticesLoader.h"
#include <iostream>

// External references - these need to be accessible from main.cpp
extern CameraController camera;
extern void updateScanBuffers();

// Static member definitions
bool InputHandler::s_leftMousePressed = false;
bool InputHandler::s_rightMousePressed = false;
double InputHandler::s_lastMouseX = 0.0;
double InputHandler::s_lastMouseY = 0.0;
bool InputHandler::s_firstMouseMove = true;

void InputHandler::initialize() {
  s_leftMousePressed = false;
  s_rightMousePressed = false;
  s_lastMouseX = 0.0;
  s_lastMouseY = 0.0;
  s_firstMouseMove = true;
}

void InputHandler::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
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

  // Arrow key panning
  if (key == GLFW_KEY_LEFT && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
    camera.pan(-10.0f, 0.0f);
    std::cout << "Pan left" << std::endl;
  }
  if (key == GLFW_KEY_RIGHT && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
    camera.pan(10.0f, 0.0f);
    std::cout << "Pan right" << std::endl;
  }
  if (key == GLFW_KEY_UP && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
    camera.pan(0.0f, 10.0f);
    std::cout << "Pan up" << std::endl;
  }
  if (key == GLFW_KEY_DOWN && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
    camera.pan(0.0f, -10.0f);
    std::cout << "Pan down" << std::endl;
  }

  // Reset pan with Space key
  if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
    if (mods & GLFW_MOD_CONTROL) {
      // Ctrl+Space: Reset rotation
      camera.resetRotation();
    }
    else {
      // Space alone: Reset pan only
      camera.resetPan();
    }
  }

  // Reset rotation with Home key
  if (key == GLFW_KEY_HOME && action == GLFW_PRESS) {
    camera.resetRotation();
  }

  // Reload scan data with R key
  if (key == GLFW_KEY_R && action == GLFW_PRESS) {
    std::cout << "Reloading scan data..." << std::endl;
    if (VerticesLoader::loadMostRecentScan(1000.0f)) {
      std::cout << "Scan data reloaded successfully!" << std::endl;
      std::cout << VerticesLoader::getScanInfo() << std::endl;
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
      updateScanBuffers();
      std::cout << "Updated 3D visualization!" << std::endl;
    }
    else {
      std::cout << "Failed to load previous scan file!" << std::endl;
    }
  }
}

void InputHandler::scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
  if (yoffset > 0) {
    camera.zoomIn(5.0f);
  }
  else if (yoffset < 0) {
    camera.zoomOut(5.0f);
  }
}

void InputHandler::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
  double xpos, ypos;
  glfwGetCursorPos(window, &xpos, &ypos);

  if (button == GLFW_MOUSE_BUTTON_RIGHT) {
    if (action == GLFW_PRESS) {
      s_rightMousePressed = true;
      s_lastMouseX = xpos;
      s_lastMouseY = ypos;
      s_firstMouseMove = true;
      std::cout << "Right mouse pressed - Rotation mode enabled" << std::endl;
    }
    else if (action == GLFW_RELEASE) {
      s_rightMousePressed = false;
      std::cout << "Right mouse released - Rotation mode disabled" << std::endl;
    }
  }
  else if (button == GLFW_MOUSE_BUTTON_MIDDLE) {
    if (action == GLFW_PRESS) {
      std::cout << "Middle mouse clicked - Zoom to fit" << std::endl;
      zoomToFit();
    }
  }
  // Removed left mouse button handling since we're not using it for panning anymore
}

void InputHandler::cursorPositionCallback(GLFWwindow* window, double xpos, double ypos) {
  // Handle mouse movement for rotation only
  if (!s_firstMouseMove) {
    double deltaX = xpos - s_lastMouseX;
    double deltaY = ypos - s_lastMouseY;

    // Only process if we have some movement
    if (abs(deltaX) > 0.1 || abs(deltaY) > 0.1) {
      if (s_rightMousePressed) {
        // Right mouse only: Rotation
        float rotDeltaX = static_cast<float>(deltaX) * 0.005f;
        float rotDeltaY = static_cast<float>(deltaY) * 0.005f;

        camera.rotate(rotDeltaX, rotDeltaY);
        // Reduced debug output  
        static int rotCounter = 0;
        if (rotCounter++ % 10 == 0) {
          std::cout << "Rotation active..." << std::endl;
        }
      }
      // Left mouse no longer does anything - removed panning
    }
  }

  // Update last mouse position
  s_lastMouseX = xpos;
  s_lastMouseY = ypos;
  s_firstMouseMove = false;
}

void InputHandler::setupCallbacks(GLFWwindow* window) {
  glfwSetKeyCallback(window, keyCallback);
  glfwSetScrollCallback(window, scrollCallback);
  glfwSetMouseButtonCallback(window, mouseButtonCallback);
  glfwSetCursorPosCallback(window, cursorPositionCallback);
}

void InputHandler::printControlInstructions() {
  std::cout << "=== Scan Data Visualization Controls ===" << std::endl;
  std::cout << "  Mouse Wheel        : Zoom In/Out" << std::endl;
  std::cout << "  Middle Mouse Click : Zoom to Fit All Data" << std::endl;
  std::cout << "  + or Numpad+       : Zoom In" << std::endl;
  std::cout << "  - or Numpad-       : Zoom Out" << std::endl;
  std::cout << "  Arrow Keys         : Pan View (←↑↓→)" << std::endl;
  std::cout << "  Right Mouse Drag   : Rotate View" << std::endl;
  std::cout << "  Space              : Reset Pan to Center" << std::endl;
  std::cout << "  Ctrl+Space         : Reset Rotation" << std::endl;
  std::cout << "  Home               : Reset Rotation" << std::endl;
  std::cout << "  Tab                : Next Scan File" << std::endl;
  std::cout << "  Shift + Tab        : Previous Scan File" << std::endl;
  std::cout << "  R                  : Reload Scan Data" << std::endl;
  std::cout << "  ESC                : Exit" << std::endl;

  auto fileInfo = VerticesLoader::getCurrentFileInfo();
  std::cout << "Available Files: " << fileInfo.second << std::endl;
  std::cout << "Current Zoom: " << camera.getZoom() << "x" << std::endl;
  std::cout << "=======================================" << std::endl;
}

void InputHandler::zoomToFit() {
  // Get the vertices to calculate bounding box
  std::vector<float> scanVertices = VerticesLoader::generateScanVertices();

  if (scanVertices.empty()) {
    std::cout << "No vertices to fit" << std::endl;
    return;
  }

  // Calculate bounding box
  float minX = scanVertices[0], maxX = scanVertices[0];
  float minY = scanVertices[1], maxY = scanVertices[1];
  float minZ = scanVertices[2], maxZ = scanVertices[2];

  for (size_t i = 0; i < scanVertices.size(); i += 3) {
    minX = std::min(minX, scanVertices[i]);
    maxX = std::max(maxX, scanVertices[i]);
    minY = std::min(minY, scanVertices[i + 1]);
    maxY = std::max(maxY, scanVertices[i + 1]);
    minZ = std::min(minZ, scanVertices[i + 2]);
    maxZ = std::max(maxZ, scanVertices[i + 2]);
  }

  // Calculate the size of the bounding box
  float sizeX = maxX - minX;
  float sizeY = maxY - minY;
  float sizeZ = maxZ - minZ;
  float maxSize = std::max({ sizeX, sizeY, sizeZ });

  // Better zoom calculation for your scan data
  // Target: fit data to roughly 400-600 pixels on screen
  float targetZoom;
  if (maxSize > 1000.0f) {
    // Large scan data - use smaller base
    targetZoom = 200.0f / maxSize;
  }
  else if (maxSize > 100.0f) {
    // Medium scan data
    targetZoom = 400.0f / maxSize;
  }
  else {
    // Small scan data
    targetZoom = 800.0f / maxSize;
  }

  // Add some padding (reduce zoom by 20%)
  targetZoom *= 0.8f;

  // Clamp to reasonable range
  targetZoom = std::max(0.5f, std::min(targetZoom, 50.0f));

  // Reset pan and set zoom
  camera.resetPan();
  camera.setZoom(targetZoom);

  std::cout << "Zoomed to fit - Size: " << maxSize << ", Zoom: " << targetZoom << "x" << std::endl;
  std::cout << "Bounding box: X[" << minX << " to " << maxX << "], Y[" << minY << " to " << maxY << "], Z[" << minZ << " to " << maxZ << "]" << std::endl;
}