#include "FluidSim2D.h"

#include "Renderer.h"
#include "imgui/imgui.h"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
namespace test {
	FluidSim2D::FluidSim2D()
		: m_Proj(glm::ortho(0.0f, 960.0f, 0.0f, 540.0f, -1.0f, 1.0f)), 
			m_View(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f))), 
			m_TranslationA(200, 200, 0), m_TranslationB(400, 200, 0), prev_time(glfwGetTime()),
			particles{std::vector<Particle>(SimulationConstants::NO_OF_PARTICLES)}
	{
		// Randomly initialise the position of the particles
		for (int i = 0; i < SimulationConstants::NO_OF_PARTICLES; ++i) {
			particles[i].position.x = (float)(std::rand()) / RAND_MAX - 0.5f;
			particles[i].position.y = (float)(std::rand()) / RAND_MAX - 0.5f;

			particles[i].velocity.x = 0.0;
			particles[i].velocity.y = -1.0;

			particles[i].acceleration.x = 0.0;
			particles[i].acceleration.y = 0.0;

			particles[i].colour.r = 0.0f;
			particles[i].colour.g = 0.5f;
			particles[i].colour.b = 1.0f;
		}

		GLCall(GL_PROGRAM_POINT_SIZE);
		GLCall(glEnable(GL_BLEND));
		GLCall(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

		m_Shader = std::make_unique<Shader>("res/shaders/Fluid.shader");
		m_VAO = std::make_unique<VertexArray>();

		m_VertexBuffer = std::make_unique<VertexBuffer>(nullptr, sizeof(Particle) * SimulationConstants::NO_OF_PARTICLES);
		VertexBufferLayout layout;
		layout.Push<float>(2);	// Position
		layout.Push<float>(2);	// Velocity
		layout.Push<float>(2);	// Acceleration
		layout.Push<float>(1);	// Density
		layout.Push<float>(1);	// Pressure
		layout.Push<float>(2);	// Force of pressure
		layout.Push<float>(2);	// Force of viscocity
		layout.Push<float>(2);	// Force of other
		layout.Push<float>(3);	// Colour

		m_VAO->AddBuffer(*m_VertexBuffer, layout);
		m_Shader->Bind();
	}
	FluidSim2D::~FluidSim2D() {}

	void FluidSim2D::UpdateSpatialHashGrid()
	{
		// Rest and fill the spatial hash grid
		std::fill(std::execution::par_unseq, spatialHash.begin(), spatialHash.end(), std::array<int, 2>{INT_MAX, INT_MAX});
		std::for_each(std::execution::par_unseq, iter_idx.begin(), iter_idx.end(),
			[&](int i) {
				const Particle& particle = particles[i];
				int coord_x = std::floor((particle.position.x + 1) / PhysicsConstants::SMOOTHING_RADIUS);
				int coord_y = std::floor((particle.position.y + 1) / PhysicsConstants::SMOOTHING_RADIUS);
				int grid_hash = (coord_x * SimulationConstants::PRIME1) ^ (coord_y * SimulationConstants::PRIME2);
				grid_hash = std::abs(grid_hash) % SimulationConstants::TABLE_SIZE;

				spatialHash[i] = { grid_hash, i };
			}
		);
		std::sort(std::execution::par_unseq, spatialHash.begin(), spatialHash.end(), std::less<std::array<int, 2>>());

		// Reset and fill the indices grid
		std::fill(std::execution::par_unseq, indices.begin(), indices.end(), -1);
		std::for_each(std::execution::par_unseq, iter_idx.begin(), iter_idx.end(),
			[&](int i) {
				int prev_hash = i == 0 ? -1 : spatialHash[i - 1][0];

				if (spatialHash[i][0] != INT_MAX &&
					spatialHash[i][0] != prev_hash) {
					indices[spatialHash[i][0]] = i;
				}
			}
		);
	}

	/*
		Uses the spatial hash to compute the density of each particle.
	*/
	void FluidSim2D::UpdateParticleDensity() 
	{
		// Iterate through all particles and calculate density
		float R2 = PhysicsConstants::SMOOTHING_RADIUS * PhysicsConstants::SMOOTHING_RADIUS;

		std::for_each(std::execution::par_unseq, iter_idx.begin(), iter_idx.end(),
			[&](int i) {
				Particle& particle = particles[i];
				particle.density = 0.0f;

				// get the grid coordinate of that particle
				int coord_x = std::floor((particle.position.x + 1) / PhysicsConstants::SMOOTHING_RADIUS);
				int coord_y = std::floor((particle.position.y + 1) / PhysicsConstants::SMOOTHING_RADIUS);

				// get surrounding grid coordinates
				for (int j = -1; j <= 1; ++j) {
					for (int k = -1; k <= 1; ++k) {
						int target_x = coord_x + j;
						int target_y = coord_y + k;

						int target_grid_hash = (target_x * SimulationConstants::PRIME1) ^ (target_y * SimulationConstants::PRIME2);
						target_grid_hash = std::abs(target_grid_hash) % SimulationConstants::TABLE_SIZE;

						int target_grid_start_idx = indices[target_grid_hash];
						if (target_grid_start_idx == -1) continue;
						while (target_grid_start_idx < spatialHash.size() &&
							spatialHash[target_grid_start_idx][0] == target_grid_hash) {
							int neighbour_id = spatialHash[target_grid_start_idx][1];
							const Particle& neighbour = particles[neighbour_id];

							glm::vec2 diff = particle.position - neighbour.position;
							float eucalidian_dist = glm::dot(diff, diff);

							if (R2 > eucalidian_dist) {
								float term = R2 - eucalidian_dist;
								particle.density += poly6_kernel * term * term * term;
							}

							target_grid_start_idx++;
						}
					}
				}
			}
		);
	}

	/*
		Computes the pressure of each particle using Tait's equation
	*/
	void FluidSim2D::UpdateParticlePressure() 
	{
		std::for_each(std::execution::par_unseq, iter_idx.begin(), iter_idx.end(), 
			[&](int i) {
				Particle& particle = particles[i];
				float density_ratio = particles[i].density / PhysicsConstants::REST_DENSITY;
				float r2 = density_ratio * density_ratio;
				float r4 = r2 * r2;
				float density_ratio7 = r4 * r2 * density_ratio;
				
				particle.pressure = std::max(PhysicsConstants::GASS_CONSTANT * (density_ratio7 - 1.0f), 0.0f);
			}
		);
	}

	/*
		Calculates the force of pressure on each particle using Debrun's spiky kernel
	*/
	void FluidSim2D::ComputePressureForce()
	{
		float R2 = PhysicsConstants::SMOOTHING_RADIUS * PhysicsConstants::SMOOTHING_RADIUS;

		std::for_each(std::execution::par_unseq, iter_idx.begin(), iter_idx.end(),
			[&](int i) {
				Particle& particle = particles[i];
				glm::vec2 f_pressure(0.0f);

				// get the grid coordinate of that particle
				int coord_x = std::floor((particle.position.x + 1) / PhysicsConstants::SMOOTHING_RADIUS);
				int coord_y = std::floor((particle.position.y + 1) / PhysicsConstants::SMOOTHING_RADIUS);

				// get surrounding grid coordinates
				for (int j = -1; j <= 1; ++j) {
					for (int k = -1; k <= 1; ++k) {
						int target_x = coord_x + j;
						int target_y = coord_y + k;

						int target_grid_hash = (target_x * SimulationConstants::PRIME1) ^ (target_y * SimulationConstants::PRIME2);
						target_grid_hash = std::abs(target_grid_hash) % SimulationConstants::TABLE_SIZE;

						int target_grid_idx = indices[target_grid_hash];
						if (target_grid_idx == -1) continue;
						while (target_grid_idx < spatialHash.size() &&
							spatialHash[target_grid_idx][0] == target_grid_hash) {
							int neighbour_idx = spatialHash[target_grid_idx][1];
							if (neighbour_idx == i) {
								target_grid_idx++;
								continue;
							}

							const Particle& neighbour = particles[neighbour_idx];
							glm::vec2 diff = particle.position - neighbour.position;
							float dist2 = glm::dot(diff, diff);

							if (dist2 < R2 && dist2 > 1e-6f) {
								// Calculate Spiky gradient
								float eucalidian_dist = sqrt(dist2);
								float term = PhysicsConstants::SMOOTHING_RADIUS - eucalidian_dist;
								glm::vec2 direction = diff / eucalidian_dist;

								glm::vec2 spiky_gradient = spiky_constant * term * term * -direction;

								// Calculate Force
								float pressure_avg = 0.5f * (particle.pressure + neighbour.pressure) / neighbour.density;
								f_pressure += PhysicsConstants::MASS * pressure_avg * spiky_gradient;
							}
							target_grid_idx++;
						}
					}
				}

				particle.F_pressure = f_pressure;
			}
		);
	}

	/*
		Update the position in RAM on the CPU side and sends that data to the GPU
	*/
	void FluidSim2D::OnUpdate(float curr_time) 
	{
		float dt = curr_time - prev_time;
		prev_time = curr_time;

		UpdateSpatialHashGrid();
		UpdateParticleDensity();
		UpdateParticlePressure();

		ComputePressureForce();

		// Upload the updated vector to the existing GPU buffer
		m_VertexBuffer->Bind();
		GLCall(glBufferSubData(GL_ARRAY_BUFFER, 0, particles.size() * sizeof(Particle), particles.data()));
	}

	void FluidSim2D::OnRender()
	{
		GLCall(glClearColor(0.0f, 0.0f, 0.0f, 1.0f));
		GLCall(glClear(GL_COLOR_BUFFER_BIT));

		Renderer renderer;
		renderer.DrawArraySphere(*m_VAO, *m_Shader, SimulationConstants::NO_OF_PARTICLES);
	}

	void FluidSim2D::OnImGuiRender()
	{
		ImGui::Text("Applicaton average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
	}
}