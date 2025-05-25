#pragma once
#include <GLFW/glfw3.h>

class InputHandler {
public:
  // Initialize input handling
  static void initialize();

  // GLFW callback functions
  static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
  static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
  static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
  static void cursorPositionCallback(GLFWwindow* window, double xpos, double ypos);

  // Setup callbacks for a window
  static void setupCallbacks(GLFWwindow* window);

  // Print control instructions
  static void printControlInstructions();

  // Zoom to fit functionality
  static void zoomToFit();

private:
  // Mouse control state
  static bool s_leftMousePressed;
  static bool s_rightMousePressed;
  static double s_lastMouseX;
  static double s_lastMouseY;
  static bool s_firstMouseMove;
};