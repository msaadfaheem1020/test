#include "camera.h"
#include "raylib.h"
#include "raymath.h"
#include "utils/logger.h"

#include <cmath>

void SimulationCamera::reset() {
  LOG_TRACE("Camera reset to default position");

  camera.position = {20.0f, 20.0f, 20.0f};
  camera.target = {0.0f, 0.0f, 0.0f};
  camera.up = {0.0f, 1.0f, 0.0f};
  camera.fovy = 45.0f;
  camera.projection = CAMERA_PERSPECTIVE;

  velocity = {0.0f, 0.0f, 0.0f};

  Vector3 direction = Vector3Subtract(camera.target, camera.position);
  direction = Vector3Normalize(direction);

  pitch = asin(direction.y);
  yaw = atan2(direction.x, direction.z);

  yaw = -PI * 3.0f / 4.0f;
  pitch = -atan2(20.0f, sqrt(20.0f * 20.0f + 20.0f * 20.0f));
}

void SimulationCamera::update_input(float dt) {
  if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
    if (!IsCursorHidden())
      DisableCursor();

    Vector2 delta = GetMouseDelta();
    yaw -= delta.x * mouse_sensitivity;
    pitch -= delta.y * mouse_sensitivity;

    if (pitch > 89.0f * DEG2RAD)
      pitch = 89.0f * DEG2RAD;
    if (pitch < -89.0f * DEG2RAD)
      pitch = -89.0f * DEG2RAD;
  } else {
    if (IsCursorHidden())
      EnableCursor();
  }

  float wheel = GetMouseWheelMove();
  if (wheel != 0) {
    Vector3 forward = {sin(yaw), 0.0f, cos(yaw)};
    if (std::abs(pitch) > 0.1f) {
      forward.y = sin(pitch);
    }
    forward = Vector3Normalize(forward);

    float scroll_impulse = wheel * move_speed * 5.0f;

    velocity = Vector3Add(velocity, Vector3Scale(forward, scroll_impulse));
  }

  Vector3 move_dir = {0};

  if (IsKeyDown(KEY_W))
    move_dir.z += 1.0f;
  if (IsKeyDown(KEY_S))
    move_dir.z -= 1.0f;

  if (IsKeyDown(KEY_A))
    move_dir.x += 1.0f;
  if (IsKeyDown(KEY_D))
    move_dir.x -= 1.0f;

  if (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_SPACE))
    move_dir.y += 1.0f;
  if (IsKeyDown(KEY_LEFT_CONTROL))
    move_dir.y -= 1.0f;

  Vector3 forward = {sin(yaw), 0.0f, cos(yaw)};
  Vector3 right = {cos(yaw), 0.0f, -sin(yaw)};

  forward = Vector3Normalize(forward);
  right = Vector3Normalize(right);

  Vector3 target_vel = {0};
  target_vel = Vector3Add(target_vel, Vector3Scale(forward, move_dir.z));
  target_vel = Vector3Add(target_vel, Vector3Scale(right, move_dir.x));
  target_vel.y += move_dir.y;

  if (Vector3Length(target_vel) > 0.1f) {
    target_vel = Vector3Normalize(target_vel);
    target_vel = Vector3Scale(target_vel, move_speed);
  }

  velocity.x += (target_vel.x - velocity.x) * friction * dt;
  velocity.y += (target_vel.y - velocity.y) * friction * dt;
  velocity.z += (target_vel.z - velocity.z) * friction * dt;
}

void SimulationCamera::update_physics(float dt) {
  camera.position = Vector3Add(camera.position, Vector3Scale(velocity, dt));

  Vector3 front;
  front.x = cos(pitch) * sin(yaw);
  front.y = sin(pitch);
  front.z = cos(pitch) * cos(yaw);
  front = Vector3Normalize(front);

  camera.target = Vector3Add(camera.position, front);
}

void SimulationCamera::update(float dt) {
  update_input(dt);
  update_physics(dt);
}
