#include "body.h"

void Body::draw(double scale) {
  Vector3 screen_pos = {(float)(position.x * scale),
                        (float)(position.y * scale),
                        (float)(position.z * scale)};

  float screen_radius = std::max(1.0f, (float)(radius * scale));
  DrawSphere(screen_pos, screen_radius, color);
}
