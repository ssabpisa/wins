//
//  node.h
//  kdtree
//
//  Created by Tanaka Katsuma on 2013/12/03.
//  Copyright (c) 2013年 Katsuma Tanaka. All rights reserved.
//

#ifndef __kdtree__node__
#define __kdtree__node__

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <iostream>
#include <limits>
#include <sstream>
#include <vector>

#include "direction.h"

using namespace wins;

namespace kdtree {
    template <typename T>
    class node {
    private:
        vector<node<T>*> neighbors_;

    public:
        T point;
        node<T> *left = NULL;
        node<T> *right = NULL;
        node<T> *parent = NULL;

        ///-----------------------------------------------------------------------
        /// @name Constructor
        ///-----------------------------------------------------------------------
        /**
         *  Initialize node having a point.
         *
         *  @param point  A point.
         *
         *  @return Initialized node instance.
         */
        node(T point) :
            neighbors_(4, NULL),
            point(point) {}

        /**
         *  Initialize node as a root of a kdtree with specified points.
         *
         *  @param points  A vector of points.
         *
         *  @return Initialized node instance.
         */
        node(std::vector<T>&& points) : node<T>(&points[0], (int)points.size(), 0) {}

        //template <typename U>
        //node(std::unique_ptr<U> *points, int size, int depth = 0) {

        //}

        /**
         *  Initialize node as a root of a kdtree with specified points.
         *
         *  @param points  An array of points.
         *  @param size    A size of the array.
         *  @param depth   The depth of current node. (For internal use)
         *
         *  @return Initialized node instance.
         */
        template <typename U>
        node(U *points, int size, int depth = 0) : neighbors_(4, NULL) {
            if (size == 1) {
                this->point = (*points).get();
                return;
            }

            // Sort points
            bool is_even = !(depth & 1);
            if (is_even) {
                std::sort(points, points + size, [](U const& a, U const& b) { return a->x < b->x; });
            } else {
                std::sort(points, points + size, [](U const& a, U const& b) { return a->y < b->y; });
            }

            // Determine a point to divide
            int median = size / 2;
            this->point = (*(points + median)).get();

            // Create left node
            this->left = new node<T>(points, median, depth + 1);
            this->left->parent = this;

            // Create right node
            if (size - median - 1 > 0) {
                this->right = new node<T>(points + median + 1, size - median - 1, depth + 1);
                this->right->parent = this;
            }
        }

        ///-----------------------------------------------------------------------
        /// @name Destructor
        ///-----------------------------------------------------------------------
        /**
         *  Delete node object.
         */
        ~node() {
            if (this->has_left_node()) delete this->left;
            if (this->has_right_node()) delete this->right;
        }

        node<T>* successor() {
          if (this->right != NULL && this->right->right != NULL) {
              node<T>* min = this->right->right;
              while (min->left != NULL && min->left->left != NULL) {
                  min = min->left->left;
              }
              return min;
          }

          node<T>* pp = NULL;
          node<T>* p = this->parent;
          if (p != NULL && p->parent != NULL) {
            pp = p->parent;
          }
          while (pp != NULL && p == pp->right)
          {
              p = pp->parent;
              pp = p == NULL ? NULL : p->parent;
          }

          return pp;
        }

        node<T>* predecessor() {
          if (this->left != NULL && this->left->left != NULL) {
              node<T>* max = this->left->left;
              while (max->right != NULL && max->right->right != NULL) {
                  max = max->right->right;
              }
              return max;
          }

          node<T>* pp = NULL;
          node<T>* p = this->parent;
          node<T>* c = this;
          if (p != NULL && p->parent != NULL) {
            pp = p->parent;
          }
          while (pp != NULL && c == p->left)
          {
              c = pp;
              p = pp->parent;
              pp = p == NULL ? NULL : p->parent;
          }

          return pp;
        }

        vector<node<T>*> neighbors() {
            if (neighbors_[DIRECTION_UP] != NULL) {
              return neighbors_;
            }
            neighbors_[DIRECTION_UP] = predecessor();
            neighbors_[DIRECTION_RIGHT] = successor();
            neighbors_[DIRECTION_DOWN] = successor();
            neighbors_[DIRECTION_LEFT] = predecessor();
            return neighbors_;
        }

        double max_neighbor_dist() {
          double max = 0;
          for (auto neighbor : neighbors()) {
            if (max < distance(neighbor)) {
              max = distance(neighbor);
            }
          }
          return max;
        }

        ///-----------------------------------------------------------------------
        /// @name Helper Methods
        ///-----------------------------------------------------------------------
        /**
         *  Indicates whether the node has the left node.
         *
         *  @return Return true if the node has the left node, and return false if the node doesn't have the left node.
         */
        inline bool has_left_node() {
            return (this->left != NULL);
        }

        /**
         *  Indicates whether the node has the right node.
         *
         *  @return Return true if the node has the right node, and return false if the node doesn't have the right node.
         */
        inline bool has_right_node() {
            return (this->right != NULL);
        }

        /**
         *  Indicates whether the node is the leaf.
         *
         *  @return Return true if the node is the leaf, and return false if the node isn't the leaf.
         */
        inline bool is_leaf() {
            return (!this->has_left_node() && !this->has_right_node());
        }

        /**
         *  Calculate a distance between the receiver and the specified node.
         *
         *  @param node  A pointer to the node.
         *
         *  @return A distance between two nodes.
         */
        inline double distance(node<T> *node) {
            double dx = this->point->x - node->point->x;
            double dy = this->point->y - node->point->y;

            return std::sqrt(dx * dx + dy * dy);
        }

        /**
         *  Calculate a distance between the receiver and the specified point.
         *
         *  @param point  A point.
         *
         *  @return A distance between the receiver and the point.
         */
        inline double distance(T point) {
            double dx = this->point->x - point->x;
            double dy = this->point->y - point->y;

            return std::sqrt(dx * dx + dy * dy);
        }

        /**
         *  Get a closer node to the receiver.
         *
         *  @param a  A pointer to node1.
         *  @param b  A pointer to node2.
         *
         *  @return The closer node to the receiver.
         */
        inline node<T> * closer(node<T> *a, node<T> *b) {
            return this->distance(a) < this->distance(b) ? a : b;
        }

        ///-----------------------------------------------------------------------
        /// @name Nearest Neighbor Search
        ///-----------------------------------------------------------------------
        /**
         *  Search for the nearest neighbor in the tree.
         *
         *  @param query_point  A query point.
         *  @param depth        The depth of current node. (For internal use)
         *
         *  @return The nearest neighbor node.
         */
        node<T> * nearest(T query_point, const int depth = 0) {
            node<T> *query = new node<T>(query_point);
            node<T> *nearest = this->nearest(query, depth);
            delete query;

            return nearest;
        }

        /**
         *  Search for the nearest neighbor in the tree.
         *
         *  @param query  A query node.
         *  @param depth  The depth of current node. (For internal use)
         *
         *  @return The nearest neighbor node.
         */
        node<T> * nearest(node<T> *query, const int depth = 0) {
            // If self is the leaf, return self
            if (this->is_leaf()) {
                return this;
            }

            // Calculate distance between the query and self along one direction (if depth is even, the direction is horizontal)
            bool is_even = !(depth & 1);
            double dx = query->point->x - this->point->x;
            double dy = query->point->y - this->point->y;
            double distance = is_even ? dx : dy;

            // Find the nearest node
            bool left = is_even ? (this->point->x > query->point->x) : (this->point->y > query->point->y);
            node<T> *leaf = NULL;

            if (left) {
                if (this->has_left_node()) {
                    leaf = this->left->nearest(query, depth + 1);

                    // Compare the found node and the nearest node in the other divided part
                    if (this->has_right_node() && distance < query->distance(leaf)) {
                        leaf = query->closer(leaf, this->right->nearest(query, depth + 1));
                    }
                } else {
                    leaf = this->right->nearest(query, depth + 1);
                }
            } else {
                if (this->has_right_node()) {
                    leaf = this->right->nearest(query, depth + 1);

                    // Compare the found leaf and the nearest leaf in the other divided part
                    if (this->has_left_node() && distance < query->distance(leaf)) {
                        leaf = query->closer(leaf, this->left->nearest(query, depth + 1));
                    }
                } else {
                    leaf = this->left->nearest(query, depth + 1);
                }
            }

            return query->closer(this, leaf);
        }

        /**
         *  Search for all nearest neighbors within a certain radius of a point.
         *
         *  @param query_point  A query point.
         *  @param r            A radius of the circle.
         *
         *  @return The vector of neighbors.
         */
        std::vector<node<T> *> radius_nearest(T query_point, const double r) {
            node<T> *query = new node<T>(query_point);
            std::vector<node<T> *> neighbors = this->radius_nearest(query, r);
            delete query;

            return neighbors;
        }

        /**
         *  Search for all nearest neighbors within a certain radius of a point.
         *
         *  @param query  A query point.
         *  @param r      A radius of the circle.
         *
         *  @return The vector of neighbors.
         */
        std::vector<node<T> *> radius_nearest(node<T> *query, const double r) {
            std::vector<node<T> *> neighbors;

            this->_radius_nearest(neighbors, query, r, 0);

            return neighbors;
        }

        void _radius_nearest(std::vector<node<T> *>& neighbors, node<T> *query, const double r, const int depth = 0) {
            // Calculate distance between the query and self along one direction (if depth is even, the direction is horizontal)
            bool is_even = !(depth & 1);
            double dx = query->point->x - this->point->x;
            double dy = query->point->y - this->point->y;
            double distance = is_even ? std::sqrt(dx * dx) : std::sqrt(dy * dy);

            // Find the node inside the circle with specified radius
            bool left = is_even ? (this->point->x > query->point->x) : (this->point->y > query->point->y);

            if (left) {
                if (this->has_left_node()) {
                    this->left->_radius_nearest(neighbors, query, r, depth + 1);
                }

                if (this->has_right_node() && distance <= r) {
                    this->right->_radius_nearest(neighbors, query, r, depth + 1);
                }
            } else {
                if (this->has_right_node()) {
                    this->right->_radius_nearest(neighbors, query, r, depth + 1);
                }

                if (this->has_left_node() && distance <= r) {
                    this->left->_radius_nearest(neighbors, query, r, depth + 1);
                }
            }

            if (query->distance(this) <= r) {
                if (neighbors.size() == 0) {
                    // Add self
                    neighbors.push_back(this);
                } else {
                    // Add self with insertion sort
                    for (typename std::vector<node<T> *>::iterator it = neighbors.begin(); it <= neighbors.end(); ++it) {
                        if (it == neighbors.end()) {
                            neighbors.push_back(this);
                            break;
                        }

                        node<T> *neighbor = *it;
                        if (query->distance(neighbor) > query->distance(this)) {
                            neighbors.insert(it, this);
                            break;
                        }
                    }
                }
            }
        }

        ///-----------------------------------------------------------------------
        /// @name k-Nearest Neighbor Search
        ///-----------------------------------------------------------------------
        /**
         *  Search for k-nearest neighbors in the tree.
         *
         *  @param query_point  A query point.
         *  @param k            Number of closest points to find.
         *
         *  @return The vector of neighbors.
         */
        std::vector<node<T> *> k_nearest(T query_point, const int k) {
            node<T> *query = new node<T>(query_point);
            std::vector<node<T> *> neighbors = this->k_nearest(query, k);
            delete query;

            return neighbors;
        }

        /**
         *  Search for k-nearest neighbors in the tree.
         *
         *  @param query  A query node.
         *  @param k      Number of closest points to find.
         *
         *  @return The vector of neighbors.
         */
        std::vector<node<T> *> k_nearest(node<T> *query, const int k) {
            std::vector<node<T> *> neighbors;

            this->_k_nearest(neighbors, query, k, 0);

            return neighbors;
        }

        void _k_nearest(std::vector<node<T> *>& neighbors, node<T> *query, const int k, const int depth = 0) {
            double max_distance;
            if (neighbors.size() > 0) {
                max_distance = query->distance(neighbors.at(neighbors.size() - 1));
            } else {
                max_distance = 0;
            }

            // Calculate distance between the query and self along one direction (if depth is even, the direction is horizontal)
            bool is_even = !(depth & 1);
            double dx = query->point->x - this->point->x;
            double dy = query->point->y - this->point->y;
            double distance = is_even ? std::sqrt(dx * dx) : std::sqrt(dy * dy);

            // Find the nearest node
            bool left = is_even ? (this->point->x > query->point->x) : (this->point->y > query->point->y);

            if (left) {
                if (this->has_left_node()) {
                    this->left->_k_nearest(neighbors, query, k, depth + 1);
                }

                // Update max distance
                if (neighbors.size() > 0) {
                    max_distance = query->distance(neighbors.at(neighbors.size() - 1));
                } else {
                    max_distance = 0;
                }

                if (this->has_right_node() && (neighbors.size() < k || distance < max_distance)) {
                    this->right->_k_nearest(neighbors, query, k, depth + 1);
                }
            } else {
                if (this->has_right_node()) {
                    this->right->_k_nearest(neighbors, query, k, depth + 1);
                }

                // Update max distance
                if (neighbors.size() > 0) {
                    max_distance = query->distance(neighbors.at(neighbors.size() - 1));
                } else {
                    max_distance = 0;
                }

                if (this->has_left_node() && (neighbors.size() < k || distance < max_distance)) {
                    this->left->_k_nearest(neighbors, query, k, depth + 1);
                }
            }

            if (neighbors.size() == 0) {
                // Add self
                neighbors.push_back(this);
            } else {
                // Add self with insertion sort
                for (typename std::vector<node<T> *>::iterator it = neighbors.begin(); it <= neighbors.end(); ++it) {
                    if (it == neighbors.end()) {
                        neighbors.push_back(this);
                        break;
                    }

                    node<T> *neighbor = *it;
                    if (query->distance(this) < query->distance(neighbor)) {
                        neighbors.insert(it, this);
                        break;
                    }
                }

                if (neighbors.size() > k) {
                    neighbors.pop_back();
                }
            }
        }
    };
}

#endif /* defined(__kdtree__node__) */
