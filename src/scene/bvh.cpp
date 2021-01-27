#include "bvh.h"

#include "CGL/CGL.h"
#include "triangle.h"

#include <iostream>
#include <stack>

using namespace std;

namespace CGL {
namespace SceneObjects {

BVHAccel::BVHAccel(const std::vector<Primitive *> &_primitives,
                   size_t max_leaf_size) {

  primitives = std::vector<Primitive *>(_primitives);
  root = construct_bvh(primitives.begin(), primitives.end(), max_leaf_size);
}

BVHAccel::~BVHAccel() {
  if (root)
    delete root;
  primitives.clear();
}

BBox BVHAccel::get_bbox() const { return root->bb; }

void BVHAccel::draw(BVHNode *node, const Color &c, float alpha) const {
  if (node->isLeaf()) {
    for (auto p = node->start; p != node->end; p++) {
      (*p)->draw(c, alpha);
    }
  } else {
    draw(node->l, c, alpha);
    draw(node->r, c, alpha);
  }
}

void BVHAccel::drawOutline(BVHNode *node, const Color &c, float alpha) const {
  if (node->isLeaf()) {
    for (auto p = node->start; p != node->end; p++) {
      (*p)->drawOutline(c, alpha);
    }
  } else {
    drawOutline(node->l, c, alpha);
    drawOutline(node->r, c, alpha);
  }
}

BVHNode *BVHAccel::construct_bvh(std::vector<Primitive *>::iterator start,
                                 std::vector<Primitive *>::iterator end,
                                 size_t max_leaf_size) {

  // TODO (Part 2.1):
  // Construct a BVH from the given vector of primitives and maximum leaf
  // size configuration. The starter code build a BVH aggregate with a
  // single leaf node (which is also the root) that encloses all the
  // primitives.

  BBox bbox;

  for (auto p = start; p != end; p++) {
    BBox bb = (*p)->get_bbox();
    bbox.expand(bb);
  }

  BVHNode *node = new BVHNode(bbox);
  node->start = start;
  node->end = end;

  size_t node_size = 0;
  for (auto it = start; it != end; ++it) {
    node_size++;
  }
  if (node_size < max_leaf_size) {
    return node;
  }

  int axis = bbox.extent[1] > bbox.extent[0]? 1: 0;
  axis = bbox.extent[2] > bbox.extent[axis]? 2: axis;
  double mid = bbox.centroid()[axis];

  auto left = new std::vector<Primitive *>();
  auto right = new std::vector<Primitive *>();
  for (auto it = start; it != end; ++it) {
    if ((*it)->get_bbox().centroid()[axis] > mid) {
      right->push_back(*it);
    }
    else {
      left->push_back(*it);
    }
  }

  if (left->size() > 0 && right->size() > 0) {
    node->l = construct_bvh(left->begin(), left->end(), max_leaf_size);
    node->r = construct_bvh(right->begin(), right->end(), max_leaf_size);
  }
  else {
    delete left;
    delete right;
  }

  return node;
}

bool BVHAccel::has_intersection(const Ray &ray, BVHNode *node) const {
  // TODO (Part 2.3):
  // Fill in the intersect function.
  // Take note that this function has a short-circuit that the
  // Intersection version cannot, since it returns as soon as it finds
  // a hit, it doesn't actually have to find the closest hit.

  double t0, t1;
  if (!node->bb.intersect(ray, t0, t1)) {
    return false;
  }

  if (node->isLeaf()) {
    for (auto it = node->start; it != node->end; ++it) {
      total_isects++;
      if ((*it)->has_intersection(ray))
        return true;
    }

    return false;
  }
  else {
    return has_intersection(ray, node->l) || has_intersection(ray, node->r);
  }
}

bool BVHAccel::intersect(const Ray &ray, Intersection *i, BVHNode *node) const {
  // TODO (Part 2.3):
  // Fill in the intersect function.

  double t0, t1;
  if (!node->bb.intersect(ray, t0, t1)) {
    return false;
  }

  bool hit = false;
  if (node->isLeaf()) {
    for (auto it = node->start; it != node->end; ++it) {
      total_isects++;
      hit = (*it)->intersect(ray, i) || hit;
    }
  }
  else {
    hit = intersect(ray, i, node->l) || hit;
    hit = intersect(ray, i, node->r) || hit;
  }
  return hit;
}

} // namespace SceneObjects
} // namespace CGL
