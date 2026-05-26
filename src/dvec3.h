#pragma once

#include <iostream>

struct DVec3 {
  double x, y, z;

  DVec3() : x(0), y(0), z(0) {}
  DVec3(double x, double y, double z) : x(x), y(y), z(z) {}

  DVec3 operator+(const DVec3 &other) const;
  DVec3 operator-(const DVec3 &other) const;
  DVec3 operator*(double scalar) const;
  DVec3 operator/(double scalar) const;

  DVec3 &operator+=(const DVec3 &other);
  DVec3 &operator-=(const DVec3 &other);

  double length_squared() const;
  double length() const;
  double distance(const DVec3 &other) const;
};

std::ostream &operator<<(std::ostream &os, const DVec3 &vec);
