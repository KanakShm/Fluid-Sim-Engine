cmake_minimum_required(VERSION 3.10)
project(SimulationEngine)

set(CMAKE_BUILD_TYPE "Release")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -s USE_GLFW=3 -s USE_WEBGL2=1 -s FULL_ES3=1 -s WASM=1 -s ALLOW_MEMORY_GROWTH=1 -O3")
set(CMAKE_EXECUTABLE_SUFFIX ".html")

file(GLOB IMGUI_CORE "src/vendor/imgui/*.cpp")
file(GLOB STB_CORE "src/vendor/stb/*.cpp")

set(SOURCES
    src/Application.cpp
    src/IndexBuffer.cpp
    src/Renderer.cpp
    src/Shader.cpp
    src/VertexBuffer.cpp
    src/VertexArray.cpp
    src/Texture.cpp

    src/simulations/FluidSim2D.cpp
    src/simulations/Simulation.cpp
    src/simulations/TestTexture2D.cpp
    src/simulations/TestClearColour.cpp

    ${IMGUI_CORE}
    ${STB_CORE}
)

include_directories(
    .
    src
    src/vendor
)

add_executable(index ${SOURCES})

set(SHELL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/shell.html")
target_link_options(index PRIVATE
    "--shell-file" "${SHELL_PATH}"
    "-sUSE_GLFW=3"
    "-sUSE_WEBGL2=1"
    "-sFULL_ES3=1"
    "-sWASM=1"
    "-sALLOW_MEMORY_GROWTH=1"
)

set_target_properties(index PROPERTIES LINK_FLAGS "--preload-file ${CMAKE_SOURCE_DIR}/res@res")