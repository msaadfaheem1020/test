#pragma once

#include "body.h"
#include "dvec3.h"
#include <vector>
class OctreeNode {
public:
  double min_x, max_x;
  double min_y, max_y;
  double min_z, max_z;

  DVec3 center_of_mass;
  double total_mass;

  Body *body;

  OctreeNode *children[8];

  OctreeNode(double min_x, double max_x, double min_y, double max_y,
             double min_z, double max_z);

  bool is_leaf();
  bool contains(Body *b);

  void subdivide();
  void insert(Body *b);

  void compute_force(Body *b, double theta, double gravitational_constant);
void collect_interactions(Body *b, double theta, std::vector<OctreeNode*>& interaction_list);
  ~OctreeNode() {
    for (int i = 0; i < 8; i++) {
      delete children[i];
    }
  }

private:
  void update_mass(Body *b);
  void apply_force(Body *b, double mass, DVec3 pos,
                   double gravitational_constant);
  int get_octant(Body *b);
};

void compute_force_vectorized(Body *target_particle, const std::vector<OctreeNode*>& interaction_list, double gravitational_constant);
