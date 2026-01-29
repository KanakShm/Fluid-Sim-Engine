[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
![C++](https://img.shields.io/badge/c++-%2300599C.svg?style=for-the-badge&logo=c%2B%2B&logoColor=white)
![OpenGL](https://img.shields.io/badge/OpenGL-%23FFFFFF.svg?style=for-the-badge&logo=opengl)
![WebAssembly](https://img.shields.io/badge/WebAssembly-654FF0?style=for-the-badge&logo=webassembly&logoColor=white)

### [🚀 TRY THE LIVE DEMO](https://kanakshm.github.io/Fluid-Sim-Engine/)

A high-performance 2D fluid physics engine built from scratch in **C++17** and **OpenGL**. The project demonstrates low-level systems programming, graphics pipeline management, 
and cross-platform compilation (Native Windows & WebAssembly).

---

## 🛠️ Tech Stack & Architecture

* **Language:** C++17 (STL, Smart Pointers, Templates)
* **Graphics API:** OpenGL 3.3 (Native) / OpenGL ES 3.0 (Web)
* **Build System:** CMake & Ninja
* **Platform Abstraction:** GLFW (Windowing), Emscripten (Web Port)
* **UI:** Dear ImGui (Immediate Mode GUI)

## 🌊 Fluid Mechanics

The engine solves the **Incompressible Navier-Stokes equations** for 2D fluid flow on an Eulerian grid. The simulation is based on the [Particle-Based Fluid Simulation for 
Interactive Applications](https://matthias-research.github.io/pages/publications/sca03.pdf) paper by Matthias Müller, David Charypar and Markus Gross.

$$\frac{\partial \mathbf{u}}{\partial t} = -(\mathbf{u} \cdot \nabla) \mathbf{u} + \nu \nabla^2 \mathbf{u} - \nabla p + \mathbf{f}$$
$$\nabla \cdot \mathbf{u} = 0$$

#### Numerical Solver Stages:
1.  **Pressure and Density (Semi-Lagrangian Scheme):**  
#### A. Density Estimation (Poly6 Kernel)
Density is computed by summing neighbour contributions using the smooth **Poly6 Kernel**.
$$\rho_i = \sum_j m_j W_{poly6}(\mathbf{r}_i - \mathbf{r}_j, h)$$

#### B. Pressure Calculation (Tait Equation of State)
To simulate a liquid rather than a gas, I use the Tait equation. This stiff equation of state rapidly increases pressure as density deviates from the rest density, resisting compression.
$$p_i = \max \left( k \left[ \left( \frac{\rho_i}{\rho_0} \right)^7 - 1 \right], 0 \right)$$

* **$\rho_0$**: Rest density.
* **$k$**: Gas stiffness constant is a variable parameter in the engine, allowing the user to change it and see the impact it has on a fluid.
* **Exponent 7**: Hardens the fluid behaviour to approximate water-like incompressibility.

#### C. Pressure Force (Spiky Kernel Gradient)
The pressure gradient is computed using the **Spiky Kernel** to prevent particle clustering (tensile instability) at the kernel centre.
$$\mathbf{f}_i^{pressure} = -\sum_j m_j \frac{p_i + p_j}{2\rho_j} \nabla W_{spiky}(\mathbf{r}_i - \mathbf{r}_j, h)$$

#### D. Viscosity Force (Viscosity Kernel Laplacian)
Viscosity dampens velocity differences using the positive-definite Laplacian of the Viscosity Kernel.
$$\mathbf{f}_i^{viscosity} = \mu \sum_j m_j \frac{\mathbf{v}_j - \mathbf{v}_i}{\rho_j} \nabla^2 W_{viscosity}(\mathbf{r}_i - \mathbf{r}_j, h)$$

## 💡 Key Implementation Details

### 1. OpenGL Rendering Pipeline
Instead of using a pre-built game engine, I implemented a fixed-timestep physics loop. This ensures the simulation remains deterministic regardless of the frame rate.
* **Batching:** Particle data is streamed to the GPU via **Vertex Buffer Objects (VBOs)** to minimize draw calls.
* **Shaders:** Custom **GLSL** vertex and fragment shaders handle the colouring and rasterisation of fluid particles.
* **State Management:** Encapsulated OpenGL state changes to prevent redundant driver calls.

### 3. WebAssembly Porting Strategy
Porting a native C++ graphics application to the web required significant architectural refactoring:
* **Loop Management:** Converted the blocking `while(!WindowShouldClose)` loop into an asynchronous callback system using `emscripten_set_main_loop` to prevent browser hanging.
* **File System:** Embedded shader assets into the virtual file system (MEMFS) during the build process.

## 📊 Optimisation Algorithms
To optimise the solver, the **Spatial Hashing** algorithm is used to bring the total time complexity of density and force calculations from $$\mathbf{O(n^2)$$ to $$\mathbf{O(n)$$. The simulation
includes a toggle for spatial hashing; turning it off results in a severe drop in frame rate. To further decrease solve times, the program has been multithreaded wherever possible. Along with
this, smart decisions about how the code is written allow the program to run as fast as possible. These include, but are not limited to:
* Efficient use of variables like `static constexpr`
* optimising the hot path to avoid `new/delete` allocations during the render loop,
* Minimising the use of square roots
* Other

## 🔧 Build Instructions

### Native (Windows/Linux)
```bash
mkdir build && cd build
cmake ..
cmake --build .
