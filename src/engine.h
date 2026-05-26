#pragma once

#include "camera.h"
#include "simulation.h"

#include <string>
#include <vector>

struct EngineState {
  bool is_paused = true;
  float time_scale = 1.0f;
  float fixed_delta_time = 0.016f; // 60hz
  int target_fps = 60;

  int selected_file_index = 0;
  std::vector<std::string> available_files;
};

class Engine {
public:
  Engine();
  ~Engine();

  void run();

private:
  void init();
  void shutdown();

  void update();
  void draw();

  SimulationCamera camera;
  EngineState state;

  Simulation simulation;
};
