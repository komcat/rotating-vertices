#include "VerticesLoader.h"

// Raw vertex data based on scan measurements (normalized relative to baseline)
const std::vector<std::vector<float>> VerticesLoader::rawScanData = {
  // Origin (baseline position)
  {0.0f, 0.0f, 0.0f},

  // All 44 normalized measurement positions (relative to baseline)
  {-0.00000352f, 0.00011983f, 0.00016176f},
  {0.00000593f, -0.00000414f, 0.00040375f},
  {0.00000031f, 0.00000112f, 0.00060301f},
  {0.00000974f, 0.00000068f, -0.00020542f},
  {0.00001073f, -0.00000732f, -0.00040544f},
  {0.00002405f, 0.00000626f, -0.00060528f},
  {0.00001581f, 0.00000202f, -0.00080674f},
  {0.00002425f, 0.00000561f, -0.00100658f},
  {0.00001497f, -0.00000979f, -0.00120624f},
  {0.00003139f, -0.00001011f, -0.00140534f},
  {0.00001248f, 0.00000337f, -0.00160654f},
  {0.00023118f, 0.00000326f, -0.00100359f},
  {0.00045918f, 0.00000593f, -0.00099762f},
  {0.00068050f, 0.00000281f, -0.00099148f},
  {0.00089811f, 0.00000516f, -0.00098395f},
  {-0.00020938f, 0.00001555f, -0.00101473f},
  {-0.00041827f, -0.00000713f, -0.00101790f},
  {-0.00062892f, -0.00000839f, -0.00101949f},
  {-0.00000418f, 0.00021455f, -0.00101326f},
  {-0.00000477f, 0.00042125f, -0.00101159f},
  {-0.00000245f, 0.00061493f, -0.00101326f},
  {-0.00002262f, -0.00014990f, -0.00102986f},
  {-0.00006782f, -0.00030785f, -0.00104760f},
  {0.00001818f, -0.00059523f, -0.00101495f},
  {-0.00003131f, -0.00006377f, -0.00092937f},
  {-0.00005644f, 0.00009698f, -0.00084238f},
  {-0.00002077f, 0.00000975f, -0.00070916f},
  {0.00002332f, 0.00002558f, -0.00111610f},
  {0.00002137f, 0.00001907f, -0.00121692f},
  {0.00002967f, 0.00002663f, -0.00131732f},
  {0.00002955f, 0.00003021f, -0.00141813f},
  {0.00012311f, 0.00003182f, -0.00111616f},
  {0.00021972f, 0.00002777f, -0.00111688f},
  {0.00032595f, 0.00002006f, -0.00111600f},
  {0.00043720f, 0.00002499f, -0.00111470f},
  {-0.00008508f, 0.00002202f, -0.00111324f},
  {-0.00018004f, 0.00002110f, -0.00111171f},
  {-0.00028394f, 0.00001985f, -0.00111343f},
  {0.00011719f, 0.00012478f, -0.00111770f},
  {0.00010975f, 0.00019167f, -0.00112402f},
  {0.00007805f, 0.00025455f, -0.00113084f},
  {0.00013634f, -0.00007647f, -0.00111655f},
  {0.00012992f, -0.00018603f, -0.00111680f},
  {0.00012778f, -0.00028447f, -0.00111806f}
};

std::vector<float> VerticesLoader::generateScanVertices() {
  std::vector<float> vertices;
  vertices.reserve(rawScanData.size() * 3);

  for (const auto& point : rawScanData) {
    vertices.insert(vertices.end(), point.begin(), point.end());
  }

  return vertices;
}

std::vector<float> VerticesLoader::generateScaledScanVertices(float scaleFactor) {
  std::vector<float> vertices;
  vertices.reserve(rawScanData.size() * 3);

  for (const auto& point : rawScanData) {
    vertices.push_back(point[0] * scaleFactor);
    vertices.push_back(point[1] * scaleFactor);
    vertices.push_back(point[2] * scaleFactor);
  }

  return vertices;
}

std::vector<unsigned int> VerticesLoader::generateScanLineIndices() {
  std::vector<unsigned int> indices;

  // Connect all points in sequence: 1->2->3->4->...->44
  // (skipping origin at index 0)
  for (unsigned int i = 1; i < static_cast<unsigned int>(rawScanData.size()) - 1; ++i) {
    indices.push_back(i);     // current vertex
    indices.push_back(i + 1); // next vertex
  }

  return indices;
}

std::vector<unsigned int> VerticesLoader::generateScanPointIndices() {
  std::vector<unsigned int> indices;

  // Generate indices for all points (including baseline at index 0)
  for (unsigned int i = 0; i < static_cast<unsigned int>(rawScanData.size()); ++i) {
    indices.push_back(i);
  }

  return indices;
}