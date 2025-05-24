#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <openglDebug.h>
#include <demoShaderLoader.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <array>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Matrix 4x4 utilities
typedef std::array<float, 16> Mat4;

Mat4 mat4Identity() {
  Mat4 result = { 0 };
  result[0] = result[5] = result[10] = result[15] = 1.0f;
  return result;
}

Mat4 mat4Perspective(float fov, float aspect, float near, float far) {
  Mat4 result = { 0 };
  float tanHalfFov = tan(fov / 2.0f);

  result[0] = 1.0f / (aspect * tanHalfFov);
  result[5] = 1.0f / tanHalfFov;
  result[10] = -(far + near) / (far - near);
  result[11] = -1.0f;
  result[14] = -(2.0f * far * near) / (far - near);

  return result;
}

Mat4 mat4LookAt(float eyeX, float eyeY, float eyeZ, float centerX, float centerY, float centerZ, float upX, float upY, float upZ) {
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

Mat4 mat4RotateY(float angle) {
  Mat4 result = mat4Identity();
  float c = cos(angle);
  float s = sin(angle);

  result[0] = c;   result[2] = s;
  result[8] = -s;  result[10] = c;

  return result;
}

#define USE_GPU_ENGINE 0
extern "C"
{
  __declspec(dllexport) unsigned long NvOptimusEnablement = USE_GPU_ENGINE;
  __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = USE_GPU_ENGINE;
}

// Generate scan vertices from your data - all 45 points
std::vector<float> generateScanVertices() {
  // Normalized vertices (baseline subtracted) - All 45 points (baseline + 44 measurements)
  // Values are normalized relative to baseline position
  std::vector<float> vertices = {
    // Origin (baseline position)
     0.0f,          0.0f,          0.0f,

     // All 44 normalized measurement positions (relative to baseline)
     -0.00000352f,   0.00011983f,   0.00016176f,
      0.00000593f,  -0.00000414f,   0.00040375f,
      0.00000031f,   0.00000112f,   0.00060301f,
      0.00000974f,   0.00000068f,  -0.00020542f,
      0.00001073f,  -0.00000732f,  -0.00040544f,
      0.00002405f,   0.00000626f,  -0.00060528f,
      0.00001581f,   0.00000202f,  -0.00080674f,
      0.00002425f,   0.00000561f,  -0.00100658f,
      0.00001497f,  -0.00000979f,  -0.00120624f,
      0.00003139f,  -0.00001011f,  -0.00140534f,
      0.00001248f,   0.00000337f,  -0.00160654f,
      0.00023118f,   0.00000326f,  -0.00100359f,
      0.00045918f,   0.00000593f,  -0.00099762f,
      0.00068050f,   0.00000281f,  -0.00099148f,
      0.00089811f,   0.00000516f,  -0.00098395f,
     -0.00020938f,   0.00001555f,  -0.00101473f,
     -0.00041827f,  -0.00000713f,  -0.00101790f,
     -0.00062892f,  -0.00000839f,  -0.00101949f,
     -0.00000418f,   0.00021455f,  -0.00101326f,
     -0.00000477f,   0.00042125f,  -0.00101159f,
     -0.00000245f,   0.00061493f,  -0.00101326f,
     -0.00002262f,  -0.00014990f,  -0.00102986f,
     -0.00006782f,  -0.00030785f,  -0.00104760f,
      0.00001818f,  -0.00059523f,  -0.00101495f,
     -0.00003131f,  -0.00006377f,  -0.00092937f,
     -0.00005644f,   0.00009698f,  -0.00084238f,
     -0.00002077f,   0.00000975f,  -0.00070916f,
      0.00002332f,   0.00002558f,  -0.00111610f,
      0.00002137f,   0.00001907f,  -0.00121692f,
      0.00002967f,   0.00002663f,  -0.00131732f,
      0.00002955f,   0.00003021f,  -0.00141813f,
      0.00012311f,   0.00003182f,  -0.00111616f,
      0.00021972f,   0.00002777f,  -0.00111688f,
      0.00032595f,   0.00002006f,  -0.00111600f,
      0.00043720f,   0.00002499f,  -0.00111470f,
     -0.00008508f,   0.00002202f,  -0.00111324f,
     -0.00018004f,   0.00002110f,  -0.00111171f,
     -0.00028394f,   0.00001985f,  -0.00111343f,
      0.00011719f,   0.00012478f,  -0.00111770f,
      0.00010975f,   0.00019167f,  -0.00112402f,
      0.00007805f,   0.00025455f,  -0.00113084f,
      0.00013634f,  -0.00007647f,  -0.00111655f,
      0.00012992f,  -0.00018603f,  -0.00111680f,
      0.00012778f,  -0.00028447f,  -0.00111806f
  };
  return vertices;
}

// Alternative version with scaling factor for better visibility
std::vector<float> generateScaledScanVertices(float scaleFactor = 1000.0f) {
  // All 45 points scaled for better visibility
  std::vector<float> vertices = {
    // Origin (baseline position)
     0.0f,          0.0f,          0.0f,

     // All 44 scaled measurement positions
     -0.00000352f * scaleFactor,   0.00011983f * scaleFactor,   0.00016176f * scaleFactor,
      0.00000593f * scaleFactor,  -0.00000414f * scaleFactor,   0.00040375f * scaleFactor,
      0.00000031f * scaleFactor,   0.00000112f * scaleFactor,   0.00060301f * scaleFactor,
      0.00000974f * scaleFactor,   0.00000068f * scaleFactor,  -0.00020542f * scaleFactor,
      0.00001073f * scaleFactor,  -0.00000732f * scaleFactor,  -0.00040544f * scaleFactor,
      0.00002405f * scaleFactor,   0.00000626f * scaleFactor,  -0.00060528f * scaleFactor,
      0.00001581f * scaleFactor,   0.00000202f * scaleFactor,  -0.00080674f * scaleFactor,
      0.00002425f * scaleFactor,   0.00000561f * scaleFactor,  -0.00100658f * scaleFactor,
      0.00001497f * scaleFactor,  -0.00000979f * scaleFactor,  -0.00120624f * scaleFactor,
      0.00003139f * scaleFactor,  -0.00001011f * scaleFactor,  -0.00140534f * scaleFactor,
      0.00001248f * scaleFactor,   0.00000337f * scaleFactor,  -0.00160654f * scaleFactor,
      0.00023118f * scaleFactor,   0.00000326f * scaleFactor,  -0.00100359f * scaleFactor,
      0.00045918f * scaleFactor,   0.00000593f * scaleFactor,  -0.00099762f * scaleFactor,
      0.00068050f * scaleFactor,   0.00000281f * scaleFactor,  -0.00099148f * scaleFactor,
      0.00089811f * scaleFactor,   0.00000516f * scaleFactor,  -0.00098395f * scaleFactor,
     -0.00020938f * scaleFactor,   0.00001555f * scaleFactor,  -0.00101473f * scaleFactor,
     -0.00041827f * scaleFactor,  -0.00000713f * scaleFactor,  -0.00101790f * scaleFactor,
     -0.00062892f * scaleFactor,  -0.00000839f * scaleFactor,  -0.00101949f * scaleFactor,
     -0.00000418f * scaleFactor,   0.00021455f * scaleFactor,  -0.00101326f * scaleFactor,
     -0.00000477f * scaleFactor,   0.00042125f * scaleFactor,  -0.00101159f * scaleFactor,
     -0.00000245f * scaleFactor,   0.00061493f * scaleFactor,  -0.00101326f * scaleFactor,
     -0.00002262f * scaleFactor,  -0.00014990f * scaleFactor,  -0.00102986f * scaleFactor,
     -0.00006782f * scaleFactor,  -0.00030785f * scaleFactor,  -0.00104760f * scaleFactor,
      0.00001818f * scaleFactor,  -0.00059523f * scaleFactor,  -0.00101495f * scaleFactor,
     -0.00003131f * scaleFactor,  -0.00006377f * scaleFactor,  -0.00092937f * scaleFactor,
     -0.00005644f * scaleFactor,   0.00009698f * scaleFactor,  -0.00084238f * scaleFactor,
     -0.00002077f * scaleFactor,   0.00000975f * scaleFactor,  -0.00070916f * scaleFactor,
      0.00002332f * scaleFactor,   0.00002558f * scaleFactor,  -0.00111610f * scaleFactor,
      0.00002137f * scaleFactor,   0.00001907f * scaleFactor,  -0.00121692f * scaleFactor,
      0.00002967f * scaleFactor,   0.00002663f * scaleFactor,  -0.00131732f * scaleFactor,
      0.00002955f * scaleFactor,   0.00003021f * scaleFactor,  -0.00141813f * scaleFactor,
      0.00012311f * scaleFactor,   0.00003182f * scaleFactor,  -0.00111616f * scaleFactor,
      0.00021972f * scaleFactor,   0.00002777f * scaleFactor,  -0.00111688f * scaleFactor,
      0.00032595f * scaleFactor,   0.00002006f * scaleFactor,  -0.00111600f * scaleFactor,
      0.00043720f * scaleFactor,   0.00002499f * scaleFactor,  -0.00111470f * scaleFactor,
     -0.00008508f * scaleFactor,   0.00002202f * scaleFactor,  -0.00111324f * scaleFactor,
     -0.00018004f * scaleFactor,   0.00002110f * scaleFactor,  -0.00111171f * scaleFactor,
     -0.00028394f * scaleFactor,   0.00001985f * scaleFactor,  -0.00111343f * scaleFactor,
      0.00011719f * scaleFactor,   0.00012478f * scaleFactor,  -0.00111770f * scaleFactor,
      0.00010975f * scaleFactor,   0.00019167f * scaleFactor,  -0.00112402f * scaleFactor,
      0.00007805f * scaleFactor,   0.00025455f * scaleFactor,  -0.00113084f * scaleFactor,
      0.00013634f * scaleFactor,  -0.00007647f * scaleFactor,  -0.00111655f * scaleFactor,
      0.00012992f * scaleFactor,  -0.00018603f * scaleFactor,  -0.00111680f * scaleFactor,
      0.00012778f * scaleFactor,  -0.00028447f * scaleFactor,  -0.00111806f * scaleFactor
  };
  return vertices;
}

// Generate line indices to connect all vertices in sequence (1->2->3->...->44)
std::vector<unsigned int> generateScanLineIndices() {
  std::vector<unsigned int> indices;

  // Connect all points in sequence: 1->2->3->4->...->44
  // (skipping origin at index 0)
  for (unsigned int i = 1; i < 44; ++i) {
    indices.push_back(i);     // current vertex
    indices.push_back(i + 1); // next vertex
  }

  return indices;
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    glfwSetWindowShouldClose(window, GLFW_TRUE);
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

  GLFWwindow* window = glfwCreateWindow(800, 600, "Scan Data Visualization", NULL, NULL);
  if (!window)
  {
    glfwTerminate();
    return -1;
  }

  glfwSetKeyCallback(window, key_callback);
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
  shader.loadShaderProgramFromFile(RESOURCES_PATH "vertex.vert", RESOURCES_PATH "fragment.frag");

  // Generate scan data with scale factor of 1000
  std::vector<float> scanVertices = generateScaledScanVertices(1000.0f);
  std::vector<unsigned int> lineIndices = generateScanLineIndices();

  // Create VAO, VBO, and EBO for the scan data
  unsigned int VAO, VBO, EBO;

  glGenVertexArrays(1, &VAO);
  glGenBuffers(1, &VBO);
  glGenBuffers(1, &EBO);

  glBindVertexArray(VAO);

  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, scanVertices.size() * sizeof(float), scanVertices.data(), GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, lineIndices.size() * sizeof(unsigned int), lineIndices.data(), GL_STATIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);

  glBindVertexArray(0); // Unbind

  // Get uniform locations
  shader.bind();
  GLint colorLocation = shader.getUniform("color");
  GLint modelLocation = shader.getUniform("model");
  GLint viewLocation = shader.getUniform("view");
  GLint projectionLocation = shader.getUniform("projection");

  // Set up projection matrix (perspective)
  float aspect = 800.0f / 600.0f;
  Mat4 projection = mat4Perspective(M_PI / 4.0f, aspect, 0.1f, 100.0f);
  glUniformMatrix4fv(projectionLocation, 1, GL_FALSE, projection.data());

  // Set up view matrix (camera position) - closer view for small data
  Mat4 view = mat4LookAt(0.5f, 0.5f, 2.0f,  // eye position - closer to see small details
    0.0f, 0.0f, 0.0f,   // look at center
    0.0f, 1.0f, 0.0f);  // up vector
  glUniformMatrix4fv(viewLocation, 1, GL_FALSE, view.data());

  // Animation variables
  float time = 0.0f;

  while (!glfwWindowShouldClose(window))
  {
    int width = 0, height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    shader.bind();

    // Create rotation matrix for animation
    Mat4 model = mat4RotateY(time);
    glUniformMatrix4fv(modelLocation, 1, GL_FALSE, model.data());

    // Set line color (green for scan data)
    glUniform3f(colorLocation, 0.0f, 1.0f, 0.0f);

    // Bind and draw the scan line path
    glBindVertexArray(VAO);
    glDrawElements(GL_LINES, lineIndices.size(), GL_UNSIGNED_INT, 0);

    glfwSwapBuffers(window);
    glfwPollEvents();

    // Control rotation speed here - smaller values = slower rotation
    time += 0.001f; // Change this value: 0.005f = very slow, 0.05f = fast
  }

  // Cleanup
  glDeleteVertexArrays(1, &VAO);
  glDeleteBuffers(1, &VBO);
  glDeleteBuffers(1, &EBO);

  return 0;
}