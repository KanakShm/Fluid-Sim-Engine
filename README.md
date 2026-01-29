[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
![C++](https://img.shields.io/badge/c++-%2300599C.svg?style=for-the-badge&logo=c%2B%2B&logoColor=white)
![OpenGL](https://img.shields.io/badge/OpenGL-%23FFFFFF.svg?style=for-the-badge&logo=opengl)
![WebAssembly](https://img.shields.io/badge/WebAssembly-654FF0?style=for-the-badge&logo=webassembly&logoColor=white)

### [🚀 TRY THE FLUID SIMULATION ENGINE](https://kanakshm.github.io/Fluid-Sim-Engine/)

A high-performance 2D fluid physics engine built from scratch in **C++17** and **OpenGL**. The project demonstrates low-level systems programming, graphics pipeline management, 
and cross-platform compilation (Native Windows & WebAssembly).

---

## 🏛️ Tech Stack

* **Language:** C++17 (STL, Smart Pointers, Templates)
* **Graphics API:** OpenGL 3.3 (Native) / OpenGL ES 3.0 (Web)
* **Build System:** CMake & Ninja
* **Platform Abstraction:** GLFW (Windowing), Emscripten (Web Port)
* **UI:** Dear ImGui (Immediate Mode GUI)

## 🌊 Fluid Mechanics

The engine solves the **Incompressible Navier-Stokes equations** for 2D fluid flow on an Eulerian grid. The simulation is based on the [Particle-Based Fluid Simulation for 
Interactive Applications](https://matthias-research.github.io/pages/publications/sca03.pdf) paper by Matthias Müller, David Charypar and Markus Gross.

$$
\frac{\partial \mathbf{u}}{\partial t} = -(\mathbf{u} \cdot \nabla) \mathbf{u} - \frac{1}{\rho} \nabla p + \frac{\mu}{\rho} \nabla^2 \mathbf{u} + \frac{\mathbf{f}}{\rho}
$$

$$
\nabla \cdot \mathbf{u} = 0
$$

**Where:**
* **$\rho$ (Rho):** Fluid density.
* **$\mathbf{u}$:** Velocity vector field.
* **$p$:** Pressure.
* **$\mu$ (Mu):** Dynamic viscosity coefficient.
* **$\mathbf{f}$:** External body forces (e.g., gravity).

#### A. Density Estimation (Poly6 Kernel)
Density is computed by summing neighbour contributions using the smooth **Poly6 Kernel**.

$$
\rho_i = \sum_j m_j W_{poly6}(\mathbf{r}_i - \mathbf{r}_j, h)
$$

#### B. Pressure Calculation (Tait Equation of State)
To simulate a liquid rather than a gas, I used the Tait equation. This stiff equation of state rapidly increases pressure as density deviates from the rest density, resisting compression.

$$p_i = \max \left( k \left[ \left( \frac{\rho_i}{\rho_0} \right)^7 - 1 \right], 0 \right)$$

* **$\rho_0$**: Rest density.
* **$k$**: Gas stiffness constant is a variable parameter in the engine, allowing the user to change it and see the impact it has on a fluid.
* **Exponent 7**: Hardens the fluid behaviour to approximate water-like incompressibility.

#### C. Pressure Force (Spiky Kernel Gradient)
The pressure gradient is computed using the **Spiky Kernel** to prevent particle clustering (tensile instability) at the kernel centre.

$$
\mathbf{f}_i^{pressure} = -\sum_j m_j \frac{p_i + p_j}{2\rho_j} \nabla W_{spiky}(\mathbf{r}_i - \mathbf{r}_j, h)
$$

#### D. Viscosity Force (Viscosity Kernel Laplacian)
Viscosity dampens velocity differences using the positive-definite Laplacian of the Viscosity Kernel.

$$
\mathbf{f}_i^{viscosity} = \mu \sum_j m_j \frac{\mathbf{v}_j - \mathbf{v}_i}{\rho_j} \nabla^2 W_{viscosity}(\mathbf{r}_i - \mathbf{r}_j, h)
$$

## 🛠️ Technical Architecture

The engine is designed as a **Golden Reference** for high-throughput computing, prioritising data-oriented design and hardware efficiency over traditional object-oriented paradigms. It employs
**Fixed-Step Integration** with a constant $\Delta t$ to ensure reproducible simulation results across different execution runs, a prerequisite for hardware-software co-verification.

### 1. Memory Hierarchy & Data Layout
To minimise memory latency and prepare for future GPGPU offloading, the engine utilises a **Structure of Arrays (SoA)** approach.
* **Cache Locality:** By storing positions and velocities in contiguous primitive arrays, the engine maximises L1/L2 cache hit rates during the integration pass.
* **SIMD Readiness:** Data structures are explicitly designed for **16-byte alignment**, facilitating efficient 128-bit SIMD (Single Instruction, Multiple Data) load/store operations.

### 2. Spatial Partitioning & Parallel Scalability
The neighbourhood search, traditionally an $O(n^2)$ bottleneck, is optimised through a **Uniform Grid Spatial Hash**.
* **Hardware-Aware Hashing:** The grid system is designed to be non-blocking, modelling the behaviour of **Atomic Operations** in a massively parallel environment.
* **Workgroup Locality:** Particles are binned into spatial cells to minimise "pointer chasing" and simulate the localised data access patterns typical of **LDS (Local Data Share)** utilisation.

https://github.com/user-attachments/assets/c6499686-5ad3-498f-a37a-2836ebec7d52

The simulation includes a toggle for spatial hashing; turning it off results in a severe drop in frame rate.

### 3. WebAssembly Porting Strategy
Porting a native C++ graphics application to the web required significant architectural refactoring:
* **Loop Management:** Converted the blocking `while(!WindowShouldClose)` loop into an asynchronous callback system using `emscripten_set_main_loop` to prevent browser hanging.
* **File System:** Embedded shader assets into the virtual file system (MEMFS) during the build process.

## ⛰︎ Future Roadmap: GPGPU Acceleration

While currently a high-performance C++ model, the architecture is designed for a seamless transition to a **GPGPU Compute Pipeline**:

1. **Cache Locality:** To be improved further.
2. **SIMD Readiness:** To be improved further.
3. **Offload Pass:** Transitioning the SPH kernels to **OpenGL 4.6 Compute Shaders**.
4. **Shared Memory Optimization:** Implementing **LDS (Local Data Share)** caching to reduce VRAM pressure during neighbor searches.
5. **Occupancy Tuning:** Analyzing **Wavefront occupancy** and SIMD divergence using the **AMD Radeon GPU Profiler (RGP)**.

## 🔧 Build Instructions

### Native (Windows/Linux)
```bash
mkdir build && cd build
cmake ..
cmake --build .
