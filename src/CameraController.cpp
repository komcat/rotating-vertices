#include "CameraController.h"
#include <algorithm>
#include <iostream>
#include <cmath>

CameraController::CameraController()
  : zoomLevel(1.0f), minZoom(0.1f), maxZoom(10.0f), baseCameraDistance(3.0f),
  rotating(false), lastMouseX(0.0), lastMouseY(0.0),
  rotationX(0.785f), rotationY(0.785f), rotationSensitivity(0.005f) {
  // Start with isometric-like angles (45 degrees)
}

void CameraController::zoomIn(float step) {
  zoomLevel = std::min(zoomLevel + step, maxZoom);
  std::cout << "Zoom: " << zoomLevel << "x" << std::endl;
}

void CameraController::zoomOut(float step) {
  zoomLevel = std::max(zoomLevel - step, minZoom);
  std::cout << "Zoom: " << zoomLevel << "x" << std::endl;
}

void CameraController::setZoom(float zoom) {
  zoomLevel = std::clamp(zoom, minZoom, maxZoom);
}

void CameraController::setMinMaxZoom(float minZoom, float maxZoom) {
  this->minZoom = minZoom;
  this->maxZoom = maxZoom;
  zoomLevel = std::clamp(zoomLevel, minZoom, maxZoom);
}

void CameraController::startRotation(double mouseX, double mouseY) {
  rotating = true;
  lastMouseX = mouseX;
  lastMouseY = mouseY;
}

void CameraController::updateRotation(double mouseX, double mouseY) {
  if (!rotating) return;

  double deltaX = mouseX - lastMouseX;
  double deltaY = mouseY - lastMouseY;

  rotationY += deltaX * rotationSensitivity; // Horizontal mouse movement = Y rotation
  rotationX += deltaY * rotationSensitivity; // Vertical mouse movement = X rotation

  // Clamp vertical rotation to prevent flipping
  rotationX = std::max(-1.4708f, std::min(rotationX, 1.4708f)); // -π/2 + 0.1 and π/2 - 0.1

  lastMouseX = mouseX;
  lastMouseY = mouseY;
}

void CameraController::endRotation() {
  rotating = false;
}

Mat4 CameraController::getViewMatrix() const {
  float distance = baseCameraDistance / zoomLevel;
  return createRotatedViewMatrix(distance);
}

Mat4 CameraController::getProjectionMatrix(float aspect, float fov, float nearPlane, float farPlane) const {
  return mat4Perspective(fov, aspect, nearPlane, farPlane);
}

Mat4 CameraController::createRotatedViewMatrix(float distance) const {
  // Calculate camera position based on rotation angles
  float eyeX = distance * cos(rotationX) * sin(rotationY);
  float eyeY = distance * sin(rotationX);
  float eyeZ = distance * cos(rotationX) * cos(rotationY);

  return mat4LookAt(eyeX, eyeY, eyeZ,  // eye position
    0.0f, 0.0f, 0.0f,  // look at center
    0.0f, 1.0f, 0.0f); // up vector
}

Mat4 CameraController::createIsometricViewMatrix(float distance) {
  // Isometric view: camera positioned at equal distance on all axes
  return mat4LookAt(distance, distance, distance,  // eye position
    0.0f, 0.0f, 0.0f,             // look at center
    0.0f, 1.0f, 0.0f);            // up vector
}

Mat4 CameraController::createPerspectiveViewMatrix(float eyeX, float eyeY, float eyeZ,
  float centerX, float centerY, float centerZ) {
  return mat4LookAt(eyeX, eyeY, eyeZ, centerX, centerY, centerZ, 0.0f, 1.0f, 0.0f);
}

Mat4 CameraController::createOrthographicProjection(float left, float right, float bottom, float top, float near, float far) {
  Mat4 result = { 0 };

  result[0] = 2.0f / (right - left);
  result[5] = 2.0f / (top - bottom);
  result[10] = -2.0f / (far - near);
  result[12] = -(right + left) / (right - left);
  result[13] = -(top + bottom) / (top - bottom);
  result[14] = -(far + near) / (far - near);
  result[15] = 1.0f;

  return result;
}

// Private helper methods
Mat4 CameraController::mat4Identity() {
  Mat4 result = { 0 };
  result[0] = result[5] = result[10] = result[15] = 1.0f;
  return result;
}

Mat4 CameraController::mat4Perspective(float fov, float aspect, float near, float far) {
  Mat4 result = { 0 };
  float tanHalfFov = tan(fov / 2.0f);

  result[0] = 1.0f / (aspect * tanHalfFov);
  result[5] = 1.0f / tanHalfFov;
  result[10] = -(far + near) / (far - near);
  result[11] = -1.0f;
  result[14] = -(2.0f * far * near) / (far - near);

  return result;
}

Mat4 CameraController::mat4LookAt(float eyeX, float eyeY, float eyeZ,
  float centerX, float centerY, float centerZ,
  float upX, float upY, float upZ) {
  // Calculate forward vector
  float forwardX = centerX - eyeX;
  float forwardY = centerY - eyeY;
  float forwardZ = centerZ - eyeZ;

  // Normalize forward
  float forwardLength = sqrt(forwardX * forwardX + forwardY * forwardY + forwardZ * forwardZ);
  forwardX /= forwardLength;
  forwardY /= forwardLength;
  forwardZ /= forwardLength;

  // Calculate right vector (cross product of forward and up)
  float rightX = forwardY * upZ - forwardZ * upY;
  float rightY = forwardZ * upX - forwardX * upZ;
  float rightZ = forwardX * upY - forwardY * upX;

  // Normalize right
  float rightLength = sqrt(rightX * rightX + rightY * rightY + rightZ * rightZ);
  rightX /= rightLength;
  rightY /= rightLength;
  rightZ /= rightLength;

  // Calculate up vector (cross product of right and forward)
  float newUpX = rightY * (-forwardZ) - rightZ * (-forwardY);
  float newUpY = rightZ * (-forwardX) - rightX * (-forwardZ);
  float newUpZ = rightX * (-forwardY) - rightY * (-forwardX);

  Mat4 result = { 0 };
  result[0] = rightX;   result[1] = newUpX;   result[2] = -forwardX;  result[3] = 0;
  result[4] = rightY;   result[5] = newUpY;   result[6] = -forwardY;  result[7] = 0;
  result[8] = rightZ;   result[9] = newUpZ;   result[10] = -forwardZ; result[11] = 0;
  result[12] = -(rightX * eyeX + rightY * eyeY + rightZ * eyeZ);
  result[13] = -(newUpX * eyeX + newUpY * eyeY + newUpZ * eyeZ);
  result[14] = -(-forwardX * eyeX + -forwardY * eyeY + -forwardZ * eyeZ);
  result[15] = 1;

  return result;
}