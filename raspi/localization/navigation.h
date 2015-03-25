#ifndef NAVIGATION_H
#define NAVIGATION_H

#include <mutex>

#include "common_utils.h"
#include "kdtree/node.hpp"
#include "point.h"

class Navigation {
 private:
  static kdtree::node<Point>* destination_node_;
  static vector<kdtree::node<Point>*>::reverse_iterator current_start_;
  static vector<kdtree::node<Point>*> current_route_;

 public:
  static mutex route_mutex;
  static bool TrySetDestinationFromCoords(string s);
  static void UpdateRoute();
};

#endif
