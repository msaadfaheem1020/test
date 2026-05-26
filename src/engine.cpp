#include "engine.h"
#include "utils/logger.h"

#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "imgui.h"
#include "raylib.h"

#include <GLFW/glfw3.h>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <vector>

Engine::Engine() {
  state.is_paused = false;
  state.time_scale = 86400.0f;
  state.fixed_delta_time = 0.016f;
  state.target_fps = 60;

  init();
}

Engine::~Engine() { shutdown(); }

void Engine::init() {
  LOG_INFO("Initializing Engine...");
  InitWindow(1280, 720, "N-Body Simulation Engine");
  SetTargetFPS(state.target_fps);

  if (std::filesystem::exists("data") &&
      std::filesystem::is_directory("data")) {
    for (const auto &entry : std::filesystem::directory_iterator("data")) {
      if (entry.path().extension() == ".csv") {
        state.available_files.push_back(entry.path().string());
      }
    }
  }

  if (!state.available_files.empty()) {
    simulation.init(state.available_files[0]);
    state.selected_file_index = 0;
  } else {
    LOG_CRITICAL("No simulation data found. Aborting.");
    exit(1);
  }

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  (void)io;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
  io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

  ImGui::StyleColorsDark();

  GLFWwindow *window = glfwGetCurrentContext();

  if (window == nullptr) {
    LOG_CRITICAL("Could not retrieve GLFW Window Handle. Aborting ImGui Init.");
    exit(1);
  }

  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init("#version 330");
  LOG_INFO("Engine initialized successfully");
}

void Engine::run() {
  while (!WindowShouldClose()) {
    update();
    draw();
  }
}

void Engine::update() {
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();

  ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(),
                               ImGuiDockNodeFlags_PassthruCentralNode);

  ImGui::Begin("Simulation Manager");
  if (ImGui::BeginTabBar("MyTabBar")) {
    if (ImGui::BeginTabItem("Controls")) {
      ImGui::Checkbox("Pause Simulation", &state.is_paused);
      ImGui::DragFloat("Time Scale", &state.time_scale, 1.0f, 0.0f, 1e12f,
                       "%.1f");
      ImGui::DragFloat("Fixed Delta Time", &state.fixed_delta_time, 0.001f,
                       0.1f);

      const char *sim_types[] = {"Naive (O(N^2))", "Barnes-Hut (O(N log N))",
                                 "RISC-V MMAP (Naive)"};
      int current_type = static_cast<int>(simulation.simulation_type);
      if (ImGui::Combo("Simulation Type", &current_type, sim_types,
                       IM_ARRAYSIZE(sim_types))) {
        simulation.simulation_type =
            static_cast<Simulation::SimulationType>(current_type);
      }

      double g = simulation.gravitational_constant;
      if (ImGui::InputDouble("Gravity (G)", &g, 0.0f, 0.0f, "%.5e")) {
        simulation.gravitational_constant = g;
      }

      if (ImGui::SliderInt("Target FPS", &state.target_fps, 10, 240)) {
        SetTargetFPS(state.target_fps);
      }
      if (ImGui::Button("Reset Camera")) {
        camera.reset();
      }

      ImGui::Separator();
      if (!state.available_files.empty()) {
        std::vector<const char *> items;
        for (const auto &file : state.available_files) {
          items.push_back(file.c_str());
        }

        if (ImGui::Combo("Dataset", &state.selected_file_index, items.data(),
                         items.size())) {
          simulation.init(state.available_files[state.selected_file_index]);
        }
      }
      ImGui::EndTabItem();
    }
    if (ImGui::BeginTabItem("Stats")) {
      ImGui::Text("FPS: %d", GetFPS());
      ImGui::Text("Frame Time: %.3f ms", GetFrameTime() * 1000.0f);
      ImGui::Separator();
      Camera3D cam = camera.get_camera();
      ImGui::Text("Camera Pos: (%.1f, %.1f, %.1f)", cam.position.x,
                  cam.position.y, cam.position.z);
      ImGui::EndTabItem();
    }
    ImGui::EndTabBar();
  }
  ImGui::End();

  if (!ImGui::GetIO().WantCaptureMouse) {
    camera.update(GetFrameTime());
  }

  if (!state.is_paused) {
    simulation.update(state.fixed_delta_time * state.time_scale);
  }
}

void Engine::draw() {
  BeginDrawing();
  ClearBackground(Color{28, 28, 28, 255}); // dark gray

  BeginMode3D(camera.get_camera());
  DrawGrid(10, 1.0f);
  simulation.draw();
  EndMode3D();

  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

  if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
    GLFWwindow *backup_current_context = glfwGetCurrentContext();
    ImGui::UpdatePlatformWindows();
    ImGui::RenderPlatformWindowsDefault();
    glfwMakeContextCurrent(backup_current_context);
  }

  EndDrawing();
}

void Engine::shutdown() {
  LOG_INFO("Shutting down Engine...");
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  CloseWindow();
}
