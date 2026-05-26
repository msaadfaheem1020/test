#pragma once

#include "dvec3.h"
#include "raylib.h"

#include <cmath>

class Body {
  friend class Simulation;

public:
  Body(DVec3 position, DVec3 velocity, DVec3 acceleration, double mass,
       Color color)
      : position(position), velocity(velocity), acceleration(acceleration),
        mass(mass), radius(std::pow(mass, 1.0 / 3.0)), color(color) {}

  void draw(double scale);

  DVec3 position;
  DVec3 velocity;
  DVec3 acceleration;
  double mass;
  double radius;
  Color color;
};