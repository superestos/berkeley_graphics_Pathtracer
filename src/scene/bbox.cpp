#include "bbox.h"

#include "GL/glew.h"

#include <algorithm>
#include <iostream>

namespace CGL {

bool BBox::intersect(const Ray& r, double& t0, double& t1) const {

  // TODO (Part 2.2):
  // Implement ray - bounding box intersection test
  // If the ray intersected the bouding box within the range given by
  // t0, t1, update t0 and t1 with the new intersection times.

  auto tx0 = std::min((max.x - r.o.x) / r.d.x, (min.x - r.o.x) / r.d.x);
  auto tx1 = std::max((max.x - r.o.x) / r.d.x, (min.x - r.o.x) / r.d.x);
  auto ty0 = std::min((max.y - r.o.y) / r.d.y, (min.y - r.o.y) / r.d.y);
  auto ty1 = std::max((max.y - r.o.y) / r.d.y, (min.y - r.o.y) / r.d.y);
  auto tz0 = std::min((max.z - r.o.z) / r.d.z, (min.z - r.o.z) / r.d.z);
  auto tz1 = std::max((max.z - r.o.z) / r.d.z, (min.z - r.o.z) / r.d.z);

  t0 = std::max(std::max(tx0, ty0), tz0);
  t1 = std::min(std::min(tx1, ty1), tz1);

  return t1 > 0 && t0 < t1;
}

void BBox::draw(Color c, float alpha) const {

  glColor4f(c.r, c.g, c.b, alpha);

  // top
  glBegin(GL_LINE_STRIP);
  glVertex3d(max.x, max.y, max.z);
  glVertex3d(max.x, max.y, min.z);
  glVertex3d(min.x, max.y, min.z);
  glVertex3d(min.x, max.y, max.z);
  glVertex3d(max.x, max.y, max.z);
  glEnd();

  // bottom
  glBegin(GL_LINE_STRIP);
  glVertex3d(min.x, min.y, min.z);
  glVertex3d(min.x, min.y, max.z);
  glVertex3d(max.x, min.y, max.z);
  glVertex3d(max.x, min.y, min.z);
  glVertex3d(min.x, min.y, min.z);
  glEnd();

  // side
  glBegin(GL_LINES);
  glVertex3d(max.x, max.y, max.z);
  glVertex3d(max.x, min.y, max.z);
  glVertex3d(max.x, max.y, min.z);
  glVertex3d(max.x, min.y, min.z);
  glVertex3d(min.x, max.y, min.z);
  glVertex3d(min.x, min.y, min.z);
  glVertex3d(min.x, max.y, max.z);
  glVertex3d(min.x, min.y, max.z);
  glEnd();

}

std::ostream& operator<<(std::ostream& os, const BBox& b) {
  return os << "BBOX(" << b.min << ", " << b.max << ")";
}

} // namespace CGL
