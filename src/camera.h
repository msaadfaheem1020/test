#pragma once

#include "raylib.h"

class SimulationCamera {
public:
  SimulationCamera() { reset(); }

  void update(float dt);
  void reset();

  Camera3D get_camera() const { return camera; }

  float move_speed = 20.0f;
  float mouse_sensitivity = 0.003f;
  float friction = 10.0f;

private:
  Camera3D camera;

  Vector3 velocity;
  float yaw;
  float pitch;

  void update_input(float dt);
  void update_physics(float dt);
};
