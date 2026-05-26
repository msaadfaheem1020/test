#include "octree.h"
#include "body.h"
#include <cmath>
#include <vector>
using namespace std;

OctreeNode::OctreeNode(double min_x, double max_x, double min_y, double max_y,
                       double min_z, double max_z)
    : min_x(min_x), max_x(max_x), min_y(min_y), max_y(max_y), min_z(min_z),
      max_z(max_z) {

  center_of_mass = {0, 0, 0};
  total_mass = 0;

  body = nullptr;

  for (int i = 0; i < 8; i++)
    children[i] = nullptr;
}

bool OctreeNode::is_leaf() {
  for (int i = 0; i < 8; i++)
    if (children[i] != nullptr)
      return false;

  return true;
}

bool OctreeNode::contains(Body *b) {
  return (b->position.x >= min_x && b->position.x <= max_x &&
          b->position.y >= min_y && b->position.y <= max_y &&
          b->position.z >= min_z && b->position.z <= max_z);
}

int OctreeNode::get_octant(Body *b) {
  double mid_x = (min_x + max_x) / 2;
  double mid_y = (min_y + max_y) / 2;
  double mid_z = (min_z + max_z) / 2;

  int idx = 0;
  if (b->position.x > mid_x)
    idx += 1;
  if (b->position.y > mid_y)
    idx += 2;
  if (b->position.z > mid_z)
    idx += 4;

  return idx;
}

void OctreeNode::subdivide() {
  double mid_x = (min_x + max_x) / 2;
  double mid_y = (min_y + max_y) / 2;
  double mid_z = (min_z + max_z) / 2;

  for (int i = 0; i < 8; i++) {
    double nx0 = (i & 1) ? mid_x : min_x;
    double nx1 = (i & 1) ? max_x : mid_x;

    double ny0 = (i & 2) ? mid_y : min_y;
    double ny1 = (i & 2) ? max_y : mid_y;

    double nz0 = (i & 4) ? mid_z : min_z;
    double nz1 = (i & 4) ? max_z : mid_z;

    children[i] = new OctreeNode(nx0, nx1, ny0, ny1, nz0, nz1);
  }
}

void OctreeNode::update_mass(Body *b) {
  double new_mass = total_mass + b->mass;

  center_of_mass =
      (center_of_mass * total_mass + b->position * b->mass) / new_mass;

  total_mass = new_mass;
}

void OctreeNode::insert(Body *b) {

  if (!contains(b))
    return;

  if (body == nullptr && is_leaf()) {
    body = b;
    total_mass = b->mass;
    center_of_mass = b->position;
    return;
  }

  if (is_leaf()) {
    Body *existing = body;
    body = nullptr;

    subdivide();
    children[get_octant(existing)]->insert(existing);
  }

  children[get_octant(b)]->insert(b);

  update_mass(b);
}

void OctreeNode::apply_force(Body *b, double mass, DVec3 pos,
                             double gravitational_constant) {

  DVec3 r = pos - b->position;

  double softening = 1e9;
  double dist_sq = r.length_squared() + softening * softening;
  double dist = sqrt(dist_sq);

  DVec3 tmp = r / (dist_sq * dist);
  tmp = tmp * gravitational_constant;

  b->acceleration += tmp * mass;
}

void OctreeNode::compute_force(Body *b, double theta,
                               double gravitational_constant) {

  if (total_mass == 0)
    return;

  if (is_leaf()) {
    if (body != nullptr && body != b)
      apply_force(b, body->mass, body->position, gravitational_constant);
    return;
  }

  double s = max_x - min_x;
  double d = b->position.distance(center_of_mass);
  if ((s / d) < theta) {
    apply_force(b, total_mass, center_of_mass, gravitational_constant);
  } else {
    for (int i = 0; i < 8; i++)
      if (children[i])
        children[i]->compute_force(b, theta, gravitational_constant);
  }
}

void OctreeNode::collect_interactions(Body *b, double theta, vector<OctreeNode*>& interaction_list) {
  if (total_mass == 0)
    return;

  if (is_leaf()) {
    if (body != nullptr && body != b) {
      interaction_list.push_back(this); 
    }
    return;
  }

  double s = max_x - min_x;
  double d = b->position.distance(center_of_mass);

  if ((s / d) < theta) {
    interaction_list.push_back(this);
  } else {
    for (int i = 0; i < 8; i++) {
      if (children[i]) {
        children[i]->collect_interactions(b, theta, interaction_list);
      }
    }
  }
}

void compute_force_vectorized(Body *target_particle, const vector<OctreeNode*>& interaction_list, double gravitational_constant) {
  int total_elements = interaction_list.size();
  if (total_elements == 0) return;

  vector<double> cluster_positions_x(total_elements);
  vector<double> cluster_positions_y(total_elements);
  vector<double> cluster_positions_z(total_elements);
  vector<double> cluster_mass_payloads(total_elements);

  for (int idx = 0; idx < total_elements; idx++) {
    cluster_positions_x[idx]    = interaction_list[idx]->center_of_mass.x;
    cluster_positions_y[idx]    = interaction_list[idx]->center_of_mass.y;
    cluster_positions_z[idx]    = interaction_list[idx]->center_of_mass.z;
    cluster_mass_payloads[idx]  = interaction_list[idx]->total_mass;
  }

  double anchor_coord_x = target_particle->position.x;
  double anchor_coord_y = target_particle->position.y;
  double anchor_coord_z = target_particle->position.z;
  
  double stability_softening = 1e9; 
  double softening_squared   = stability_softening * stability_softening;

  double accumulated_pull_x = 0.0;
  double accumulated_pull_y = 0.0;
  double accumulated_pull_z = 0.0;

  for (int idx = 0; idx < total_elements; idx++) {
    double delta_x = cluster_positions_x[idx] - anchor_coord_x;
    double delta_y = cluster_positions_y[idx] - anchor_coord_y;
    double delta_z = cluster_positions_z[idx] - anchor_coord_z;

    double separation_squared = (delta_x * delta_x) + (delta_y * delta_y) + (delta_z * delta_z) + softening_squared;
    double true_distance      = sqrt(separation_squared);
    
    double inverse_cubic_distance = 1.0 / (separation_squared * true_distance);
    double localized_force_scalar = gravitational_constant * cluster_mass_payloads[idx] * inverse_cubic_distance;

    accumulated_pull_x += localized_force_scalar * delta_x;
    accumulated_pull_y += localized_force_scalar * delta_y;
    accumulated_pull_z += localized_force_scalar * delta_z;
  }

  target_particle->acceleration.x += accumulated_pull_x;
  target_particle->acceleration.y += accumulated_pull_y;
  target_particle->acceleration.z += accumulated_pull_z;
}
