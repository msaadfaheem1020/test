#include "dvec3.h"

#include <cmath>

double DVec3::length_squared() const { return x * x + y * y + z * z; }

double DVec3::length() const { return std::sqrt(length_squared()); }

double DVec3::distance(const DVec3 &other) const {
  return std::sqrt((x - other.x) * (x - other.x) +
                   (y - other.y) * (y - other.y) +
                   (z - other.z) * (z - other.z));
}

DVec3 DVec3::operator+(const DVec3 &other) const {
  return DVec3(x + other.x, y + other.y, z + other.z);
}

DVec3 DVec3::operator-(const DVec3 &other) const {
  return DVec3(x - other.x, y - other.y, z - other.z);
}

DVec3 DVec3::operator*(double scalar) const {
  return DVec3(x * scalar, y * scalar, z * scalar);
}

DVec3 DVec3::operator/(double scalar) const {
  return DVec3(x / scalar, y / scalar, z / scalar);
}

DVec3 &DVec3::operator+=(const DVec3 &other) {
  x += other.x;
  y += other.y;
  z += other.z;
  return *this;
}

DVec3 &DVec3::operator-=(const DVec3 &other) {
  x -= other.x;
  y -= other.y;
  z -= other.z;
  return *this;
}

std::ostream &operator<<(std::ostream &os, const DVec3 &vec) {
  os << "(" << vec.x << ", " << vec.y << ", " << vec.z << ")";
  return os;
}
