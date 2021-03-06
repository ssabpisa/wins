#include <cassert>
#include <queue>
#include <stdexcept>

#include "global.h"
#include "location.h"
#include "map.h"
#include "navigation.h"

namespace wins {

using namespace std;

#define MULTIPLIER 10
#define NEIGHBOR_RADIUS 2.5

struct PriorityNode {
  kdtree::node<Point*>* node;
  double priority;
};

kdtree::node<Point*>* Navigation::destination_node_ = nullptr;
vector<kdtree::node<Point*>*>::reverse_iterator Navigation::current_start_;
vector<kdtree::node<Point*>*> Navigation::current_route_;
unordered_set<kdtree::node<Point*>*> Navigation::nearby_route_;
bool Navigation::navigating_ = false;
mutex Navigation::route_mutex;

bool Navigation::TrySetDestinationFromCoords(string s) {
  vector<string> coords = split(s, ',');
  if (coords.size() < 2) {
    return false;
  }
  double in_xd;
  double in_yd;
  try {
    in_xd = stod(coords[0]);
    in_yd = stod(coords[1]);
  } catch(...) {
    return false;
  }
  kdtree::node<Point*>* n = Map::NodeNearest(stod(coords[0]), stod(coords[1]));
  int in_xi = (int)(in_xd * MULTIPLIER);
  int in_yi = (int)(in_yd * MULTIPLIER);
  int n_xi = (int)(n->point->x * MULTIPLIER);
  int n_yi = (int)(n->point->y * MULTIPLIER);

  if (in_xi == n_xi and in_yi == n_yi) {
    destination_node_ = n;
    navigating_ = true;
    return true;
  }
  return false;
}

void Navigation::ResetDestination() {
  destination_node_ = nullptr;
}

kdtree::node<Point*>* const Navigation::GetDestination() {
  return destination_node_;
}

void Navigation::UpdateRoute() {
  if (not navigating_) {
    return;
  }

  // Current node on map.
  auto current_node = Location::GetCurrentNode();
  if (current_node == NULL) {
    lock_guard<mutex> lock(route_mutex);
    current_start_ = current_route_.rend();
    return;
  }

  // Check if current point is in route cache.
  if (current_route_.size() > 0 and nearby_route_.count(current_node)) {
    if (current_node->distance(destination_node_) < NEIGHBOR_RADIUS) {
      Global::SetEventFlag(WINS_EVENT_DEST_REACHED);
      navigating_ = false;
      return;
    }
    auto node_in_path_iter = find(current_start_, current_route_.rend(),
        current_node);
    if (node_in_path_iter != current_route_.rend()) {
      lock_guard<mutex> lock(route_mutex);
      current_start_ = node_in_path_iter;
      return;
    }
    //  for (auto node_iter = current_route_.rbegin();
    //      node_iter != current_route_.rend();
    //      node_iter++) {
    //    bool found = false;
    //    for (auto nearby : Map::NodesInRadius(*node_iter, NEIGHBOR_RADIUS)) {
    //      if (nearby == current_node) {
    //        lock_guard<mutex> lock(route_mutex);
    //        current_start_ = node_iter;
    //        found = true;
    //        break;
    //      }
    //    }
    //    if (found)
    //      break;
    //  }
    //}
  }

  auto target_node = destination_node_;
  assert(target_node != nullptr);

  auto compareFunc = [](PriorityNode a, PriorityNode b) {
    return a.priority > b.priority;
  };
  typedef priority_queue<PriorityNode, vector<PriorityNode>,
      decltype(compareFunc)> pqueue;

  unordered_map<kdtree::node<Point*>*, kdtree::node<Point*>*> came_from;
  unordered_map<kdtree::node<Point*>*, double> cost_so_far;
  pqueue frontier(compareFunc);

  frontier.push({ Location::GetCurrentNode(), 0 });
  came_from[current_node] = current_node;
  cost_so_far[current_node] = 0;

  while (not frontier.empty()) {
    // Current iteration node.
    auto current = frontier.top();
    frontier.pop();

    if (current.node == target_node) {
      break;
    }

    auto neighbors = Map::NodesInRadius(current.node,
        NEIGHBOR_RADIUS * Global::Scale);
    auto rem_iter = neighbors.begin();
    for (auto n = neighbors.begin(); n != neighbors.end(); ++n) {
      if (*n == current.node) {
        rem_iter = n;
        //cout << "Removed current\n";
      }
    }
    neighbors.erase(rem_iter);
    //printf("%3.0f %3.0f\n", current.node->point->x, current.node->point->y);
    for (size_t i = 0; i < neighbors.size(); ++i) {
      kdtree::node<Point*>* next = neighbors[i];
      double new_cost = cost_so_far[current.node] + current.node->distance(next);
      //printf("%3.0f %3.0f %5.3f", next->point->x, next->point->y, new_cost);
      if (cost_so_far.count(next) == 0 ||
          new_cost < cost_so_far[next]) {
        cost_so_far[next] = new_cost;
        double priority = new_cost + next->distance(target_node);
        //printf(" %5.3f", priority);
        frontier.push(PriorityNode{ next, priority });
        came_from[next] = current.node;
      }
      //cout << "\n";
    }
    //cout << "\n";
  }

  lock_guard<mutex> lock(route_mutex);
  current_route_.clear();
  nearby_route_.clear();
  for (auto node = target_node; node != current_node; node = came_from[node]) {
    current_route_.push_back(node);
    for (auto nearby : Map::NodesInRadius(node, NEIGHBOR_RADIUS)) {
      nearby_route_.insert(nearby);
    }
  }
  current_route_.push_back(current_node);
  for (auto nearby : Map::NodesInRadius(current_node, NEIGHBOR_RADIUS)) {
    nearby_route_.insert(nearby);
  }

  current_start_ = current_route_.rbegin();
  Global::SetEventFlag(WINS_EVENT_ROUTE_CHANGE);
}

vector<kdtree::node<Point*>*>::const_reverse_iterator Navigation::route_begin() {
  return current_route_.rbegin();
}

vector<kdtree::node<Point*>*>::const_reverse_iterator Navigation::route_end() {
  return current_route_.rend();
}

vector<kdtree::node<Point*>*>::const_reverse_iterator Navigation::current_begin() {
  return current_start_;
}

}
