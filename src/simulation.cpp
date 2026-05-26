#include "simulation.h"
#include "octree.h"
#include "raylib.h"
#include "utils/logger.h"
#include <vector>   
#include <chrono>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <sstream>
#include <sys/mman.h>
#include <unistd.h>
#include <iostream>  
#include <iomanip>   
using namespace std;

Simulation::Simulation() {}
Simulation::~Simulation() { cleanup_mmap(); }

void Simulation::init(const std::string &filepath) {
  cleanup_mmap(false); // signal reload

  bodies.clear();
  load_bodies_from_csv(filepath);

  compute_accelerations();

  setup_mmap();
}

void Simulation::load_bodies_from_csv(const std::string &filepath) {
  LOG_INFO("Loading bodies from: {}", filepath);
  std::ifstream file(filepath);

  if (!file.is_open()) {
    LOG_ERROR("Failed to open file: {}", filepath);
    return;
  }

  std::string line;
  std::getline(file, line);

  int count = 0;
  while (std::getline(file, line)) {
    std::stringstream ss(line);
    std::string cell;
    std::vector<double> values;

    while (std::getline(ss, cell, ',')) {
      try {
        values.push_back(std::stod(cell));
      } catch (...) {
        values.push_back(0.0);
      }
    }

    // values[0] = mass, values[1..3] = position, values[4..6] = velocity
    if (values.size() >= 7) {
      bodies.emplace_back(DVec3{values[1], values[2], values[3]},
                          DVec3{values[4], values[5], values[6]},
                          DVec3{0, 0, 0}, values[0], BLUE);
      count++;
    } else {
      LOG_ERROR("Invalid row: {}", line);
    }
  }

  LOG_INFO("Loaded {} bodies from CSV", count);
}

void Simulation::compute_accelerations() {
  for (auto &b : bodies) {
    b.acceleration = {0.0, 0.0, 0.0};
  }

  if (simulation_type == SimulationType::NAIVE) {
    const double softening = 1e9;
    for (size_t i = 0; i < bodies.size(); ++i) {
      for (size_t j = i + 1; j < bodies.size(); ++j) {
        DVec3 r = bodies[j].position - bodies[i].position;

        double r2 = r.length_squared() + softening * softening;
        double inv_r = 1.0 / std::sqrt(r2);
        double inv_r3 = inv_r * inv_r * inv_r;

        DVec3 force = r * (gravitational_constant * inv_r3);

        bodies[i].acceleration += force * bodies[j].mass;
        bodies[j].acceleration -= force * bodies[i].mass;
      }
    }
  } else if (simulation_type == SimulationType::BARNES_HUT) {
    OctreeNode root(-1e12, 1e12, -1e12, 1e12, -1e12, 1e12);
    for (auto &b : bodies) {
      root.insert(&b);
    }

    // for (auto &b : bodies) {
    //   root.compute_force(&b, 0.5, gravitational_constant);
    // }
    vector<DVec3> scalar_reference_accelerations(bodies.size());

    auto start_scalar = chrono::high_resolution_clock::now();
    for (size_t idx = 0; idx < bodies.size(); ++idx) {
      bodies[idx].acceleration = DVec3(0.0, 0.0, 0.0);
      root.compute_force(&bodies[idx], 0.5, gravitational_constant);
      scalar_reference_accelerations[idx] = bodies[idx].acceleration;
    }
    auto end_scalar = chrono::high_resolution_clock::now();
    chrono::duration<double> duration_scalar = end_scalar - start_scalar;

    auto start_vectorized = chrono::high_resolution_clock::now();
    for (size_t idx = 0; idx < bodies.size(); ++idx) {
      bodies[idx].acceleration = DVec3(0.0, 0.0, 0.0);

      vector<OctreeNode*> interaction_list;
      root.collect_interactions(&bodies[idx], 0.5, interaction_list);
      compute_force_vectorized(&bodies[idx], interaction_list, gravitational_constant);
    }
    auto end_vectorized = chrono::high_resolution_clock::now();
    chrono::duration<double> duration_vectorized = end_vectorized - start_vectorized;

    double maximum_observed_discrepancy = 0.0;
    for (size_t idx = 0; idx < bodies.size(); ++idx) {
      double error_x = abs(bodies[idx].acceleration.x - scalar_reference_accelerations[idx].x);
      double error_y = abs(bodies[idx].acceleration.y - scalar_reference_accelerations[idx].y);
      double error_z = abs(bodies[idx].acceleration.z - scalar_reference_accelerations[idx].z);

      if (error_x > maximum_observed_discrepancy) maximum_observed_discrepancy = error_x;
      if (error_y > maximum_observed_discrepancy) maximum_observed_discrepancy = error_y;
      if (error_z > maximum_observed_discrepancy) maximum_observed_discrepancy = error_z;
    }

    double seconds_scalar     = duration_scalar.count();
    double seconds_vectorized = duration_vectorized.count();
    double factor_speedup     = seconds_scalar / seconds_vectorized;

    // asked ai to make these output terminal 

    cout << "\n======================================================\n";
    cout << " BARNES-HUT ENGINE KERNEL EXECUTION METRICS          \n";
    cout << " Simulation Context Size: N = " << bodies.size() << " particles\n";
    cout << "======================================================\n";
    cout << fixed << setprecision(6);
    cout << " Old Scalar Runtime        : " << seconds_scalar << " seconds\n";
    cout << " New Vectorized Array Loop : " << seconds_vectorized << " seconds\n";
    cout << setprecision(2);
    cout << " Achieved Scaling Velocity : " << factor_speedup << "x faster\n";
    cout << "------------------------------------------------------\n";
    cout << scientific << setprecision(14);
    cout << " Peak Absolute Error Trace : " << maximum_observed_discrepancy << "\n";
    cout << "======================================================\n\n";
  }
  
}

void Simulation::update(double dt) {
  double max_dist_sq = 0;

  if (simulation_type == SimulationType::MMAP) {
    if (!mmap_ptr) {
      LOG_ERROR("MMAP not set up");
      return;
    }

    MmapHeader *header = static_cast<MmapHeader *>(mmap_ptr);

    if (header->status.load(std::memory_order_acquire) == MmapStatus::IDLE) {
      Body *mapped_bodies = reinterpret_cast<Body *>(header + 1);

      // load bodies from the last step
      std::memcpy(bodies.data(), mapped_bodies, bodies.size() * sizeof(Body));

      // prepare next step
      header->dt = dt;
      header->gravity = gravitational_constant;
      std::memcpy(mapped_bodies, bodies.data(), bodies.size() * sizeof(Body));

      // signal compute
      header->status.store(MmapStatus::COMPUTE, std::memory_order_release);
    }

    for (const auto &b : bodies) {
      max_dist_sq = std::max(max_dist_sq, b.position.length_squared());
    }

  } else {
    // half step velocity update
    for (auto &b : bodies) {
      b.velocity += b.acceleration * (dt / 2.0);
    }

    // full step position update
    for (auto &b : bodies) {
      b.position += b.velocity * dt;
    }

    // compute accelerations at new positions
    compute_accelerations();

    // half step velocity update
    for (auto &b : bodies) {
      b.velocity += b.acceleration * (dt / 2.0);
    }

    for (const auto &b : bodies) {
      max_dist_sq = std::max(max_dist_sq, b.position.length_squared());
    }
  }

  double max_dist = std::sqrt(max_dist_sq);
  view_scale = 100.0 / std::max(max_dist, 1.0);
}

void Simulation::draw() {
  for (Body &body : bodies)
    body.draw(view_scale);
}

void Simulation::setup_mmap() {
  if (bodies.empty())
    return;

  if (mmap_ptr) {
    LOG_ERROR("MMAP already set up.");
    return;
  }

  mmap_size = sizeof(MmapHeader) + bodies.size() * sizeof(Body);

  mmap_fd = open("nbody_mmap.bin", O_RDWR | O_CREAT | O_TRUNC, 0666);
  if (mmap_fd == -1) {
    LOG_ERROR("Failed to open nbody_mmap.bin.");
    return;
  }

  if (ftruncate(mmap_fd, mmap_size) == -1) {
    LOG_ERROR("ftruncate failed.");
    close(mmap_fd);
    mmap_fd = -1;
    return;
  }

  mmap_ptr =
      mmap(nullptr, mmap_size, PROT_READ | PROT_WRITE, MAP_SHARED, mmap_fd, 0);
  if (mmap_ptr == MAP_FAILED) {
    LOG_ERROR("mmap failed.");
    mmap_ptr = nullptr;
    close(mmap_fd);
    mmap_fd = -1;
    return;
  }

  MmapHeader *header = static_cast<MmapHeader *>(mmap_ptr);
  header->num_bodies = static_cast<int>(bodies.size());
  header->dt = 0;
  header->gravity = gravitational_constant;
  header->status.store(MmapStatus::IDLE, std::memory_order_seq_cst);

  Body *mapped_bodies = reinterpret_cast<Body *>(header + 1);
  std::memcpy(mapped_bodies, bodies.data(), bodies.size() * sizeof(Body));

  LOG_INFO("MMAP ready: {} bodies, {} bytes.", bodies.size(), mmap_size);
}

void Simulation::cleanup_mmap(bool exiting) {
  if (mmap_ptr) {
    static_cast<MmapHeader *>(mmap_ptr)->status.store(
        exiting ? MmapStatus::EXIT : MmapStatus::RELOAD,
        std::memory_order_seq_cst);
  }

  if (mmap_ptr) {
    munmap(mmap_ptr, mmap_size);
    mmap_ptr = nullptr;
    mmap_size = 0;
  }

  if (mmap_fd != -1) {
    close(mmap_fd);
    mmap_fd = -1;
  }
}
