#ifndef GRAPH_H
#define GRAPH_H

#include <cassert>
#include <unordered_map>

#include "common_utils.h"
#include "cereal/cereal.hpp"
#include "cereal/types/string.hpp"
#include "cereal/types/unordered_map.hpp"
#include "cereal/types/vector.hpp"

#define GRAPH_VERSION 1

struct MacInfo {
  float mean;
  float std;

  template<class Archive>
  void serialize(Archive & archive) {
    archive(mean, std);
  }
};

struct Point {
  float x;
  float y;
  unordered_map<string, MacInfo> info;

  template<class Archive>
  void serialize(Archive & archive, uint32_t const version) {
    assert(version == GRAPH_VERSION);
    archive(x, y, info);
  }
};

CEREAL_CLASS_VERSION(Point, GRAPH_VERSION);

#endif // GRAPH_H
