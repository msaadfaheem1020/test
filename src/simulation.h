#pragma once

#include "body.h"
#include <atomic>
#include <string>
#include <vector>

enum class MmapStatus : int { IDLE = 0, COMPUTE = 1, RELOAD = 2, EXIT = 3 };

struct MmapHeader {
  int num_bodies;
  int pad0;
  double dt;
  double gravity;
  std::atomic<MmapStatus> status;
  char pad1[36];
};

static_assert(sizeof(MmapHeader) == 64, "MmapHeader must be exactly 64 bytes");

class Simulation {
public:
  Simulation();
  ~Simulation();

  void init(const std::string &filepath);
  void update(double dt);
  void draw();
  void load_bodies_from_csv(const std::string &filepath);

  double gravitational_constant = 6.67430e-11;

  enum class SimulationType { NAIVE, BARNES_HUT, MMAP };
  SimulationType simulation_type = SimulationType::BARNES_HUT;

private:
  void compute_accelerations();
  void setup_mmap();
  void cleanup_mmap(bool exiting = true);

  double view_scale = 1.0;
  std::vector<Body> bodies;

  int mmap_fd = -1;
  size_t mmap_size = 0;
  void *mmap_ptr = nullptr;
};
