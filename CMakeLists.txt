cmake_minimum_required(VERSION 3.28)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(nbody)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES    OFF CACHE BOOL "" FORCE)

add_subdirectory(external/raylib)
add_subdirectory(external/spdlog)

file(GLOB_RECURSE SOURCES "src/*.cpp")
list(REMOVE_ITEM SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/nbody_riscv.cpp")

set(IMGUI_DIR "external/imgui")
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

set(GLFW_INCLUDE_DIR "external/raylib/src/external/glfw/include")

add_executable(nbody ${SOURCES} ${IMGUI_SOURCES})

target_include_directories(nbody PRIVATE 
    ${CMAKE_SOURCE_DIR}/src
    ${IMGUI_DIR} 
    ${IMGUI_DIR}/backends
    ${GLFW_INCLUDE_DIR}
)

target_link_libraries(nbody PRIVATE raylib spdlog)
