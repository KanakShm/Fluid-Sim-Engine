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
			float x = (i % Init::PPR) * Init::SPACING_X / Init::PPR + Init::START_X;
			float y = (i / Init::PPR) * Init::SPACING_Y / Init::PPR + Init::START_Y;

			x += ((std::rand() % 100) / 100.0f) * 0.01f;
			y += ((std::rand() % 100) / 100.0f) * 0.01f;

			particles[i].position = glm::vec2(x, y);

			particles[i].density = 0.0f;
			particles[i].pressure = 0.0f;

			particles[i].F_pressure = glm::vec2(0.0f);
			particles[i].F_viscosity = glm::vec2(0.0f);
			particles[i].F_other = glm::vec2(0.0f);

			particles[i].colour = glm::vec3(0.0f, 0.5f, 1.0f);
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

				unsigned int hash_x = coord_x * SimulationConstants::PRIME1;
				unsigned int hash_y = coord_y * SimulationConstants::PRIME2;

				unsigned int raw_hash = hash_x ^ hash_y;
;				int grid_hash = raw_hash % SimulationConstants::TABLE_SIZE;

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
		// MOVE KERNAL CONSTANT HERE
		for (int i = 0; i < particles.size(); ++i) {
			Particle& particle = particles[i];
			particle.density = 0.0f;

			for (int j = 0; j < particles.size(); ++j) {
				Particle& neighbour = particles[j];
				glm::vec2 diff = particle.position - neighbour.position;
				float dist2 = glm::dot(diff, diff);

				if (R2 > dist2) {
					float term = R2 - dist2;
					particle.density += PhysicsConstants::MASS * PhysicsConstants::Poly6Kernal() * term * term * term;
				}
			}
		}
	}

	void FluidSim2D::UpdateParticleDensitySHG()
	{
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

						unsigned int hash_x = target_x * SimulationConstants::PRIME1;
						unsigned int hash_y = target_y * SimulationConstants::PRIME2;

						unsigned int raw_hash = hash_x ^ hash_y;
						int target_grid_hash = raw_hash % SimulationConstants::TABLE_SIZE;

						int target_grid_start_idx = indices[target_grid_hash];
						if (target_grid_start_idx == -1) continue;
						while (target_grid_start_idx < spatialHash.size() &&
							spatialHash[target_grid_start_idx][0] == target_grid_hash) {
							int neighbour_id = spatialHash[target_grid_start_idx][1];
							const Particle& neighbour = particles[neighbour_id];

							glm::vec2 diff = particle.position - neighbour.position;
							float dist2 = glm::dot(diff, diff);

							if (R2 > dist2) {
								float term = R2 - dist2;
								particle.density += PhysicsConstants::MASS * PhysicsConstants::Poly6Kernal() * term * term * term;
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
		Calculates the force of pressure on each particle using Debrun's spiky kernel.
	*/
	void FluidSim2D::ComputeForces()
	{
		float R2 = PhysicsConstants::SMOOTHING_RADIUS * PhysicsConstants::SMOOTHING_RADIUS;
		for (int i = 0; i < particles.size(); ++i) {
			Particle& particle = particles[i];
			glm::vec2 f_pressure(0.0f);
			glm::vec2 f_viscosity(0.0f);

			for (int j = 0; j < particles.size(); ++j) {
				const Particle& neighbour = particles[j];
				glm::vec2 diff = particle.position - neighbour.position;
				float dist2 = glm::dot(diff, diff);

				if (dist2 < R2 && dist2 > 1e-6f) {
					// Calculate Spiky gradiant and Laplacian of Viscosity
					float eucalidian_dist = sqrt(dist2);
					float term = PhysicsConstants::SMOOTHING_RADIUS - eucalidian_dist;
					glm::vec2 direction = diff / eucalidian_dist;

					glm::vec2 spiky_gradient = PhysicsConstants::SpikeyConstant() * term * term * direction;
					const float viscosity_laplacian = PhysicsConstants::MullerConstant() * term;

					// Calculate Forces
					float pressure_avg = 0.5f * (particle.pressure + neighbour.pressure) / neighbour.density;
					f_pressure += -PhysicsConstants::MASS * pressure_avg * spiky_gradient;

					glm::vec2 v_rel = neighbour.velocity - particle.velocity;
					f_viscosity += PhysicsConstants::MASS * (v_rel / neighbour.density) * viscosity_laplacian;
				}
			}
			particle.F_pressure = f_pressure;
			particle.F_viscosity = PhysicsConstants::VISCOCITY_COEFFICIENT * f_viscosity;
		}
	}

	/*
		Update the pressure and viscous forces of each particle using an optimised
		spatial hash grid approach and Debrun's spiky kernel.
	*/

	void FluidSim2D::ComputeForcesSHG()
	{
		float R2 = PhysicsConstants::SMOOTHING_RADIUS * PhysicsConstants::SMOOTHING_RADIUS;

		std::for_each(std::execution::par_unseq, iter_idx.begin(), iter_idx.end(),
			[&](int i) {
				Particle& particle = particles[i];
				glm::vec2 f_pressure(0.0f);
				glm::vec2 f_viscosity(0.0f);

				// get the grid coordinate of that particle
				int coord_x = std::floor((particle.position.x + 1) / PhysicsConstants::SMOOTHING_RADIUS);
				int coord_y = std::floor((particle.position.y + 1) / PhysicsConstants::SMOOTHING_RADIUS);

				// get surrounding grid coordinates
				for (int j = -1; j <= 1; ++j) {
					for (int k = -1; k <= 1; ++k) {
						int target_x = coord_x + j;
						int target_y = coord_y + k;

						unsigned int hash_x = target_x * SimulationConstants::PRIME1;
						unsigned int hash_y = target_y * SimulationConstants::PRIME2;

						unsigned int raw_hash = hash_x ^ hash_y;
						int target_grid_hash = raw_hash % SimulationConstants::TABLE_SIZE;

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
								// Calculate Spiky gradiant and Laplacian of Viscosity
								float eucalidian_dist = sqrt(dist2);
								float term = PhysicsConstants::SMOOTHING_RADIUS - eucalidian_dist;
								glm::vec2 direction = diff / eucalidian_dist;

								glm::vec2 spiky_gradient = PhysicsConstants::SpikeyConstant() * term * term * direction;
								const float viscosity_laplacian = PhysicsConstants::MullerConstant() * term;

								// Calculate Forces
								float pressure_avg = 0.5f * (particle.pressure + neighbour.pressure) / neighbour.density;
								f_pressure += -PhysicsConstants::MASS * pressure_avg * spiky_gradient;

								glm::vec2 v_rel = neighbour.velocity - particle.velocity;
								f_viscosity += PhysicsConstants::MASS * (v_rel / neighbour.density) * viscosity_laplacian;
							}
							target_grid_idx++;
						}
					}
				}

				particle.F_pressure = f_pressure;
				particle.F_viscosity = PhysicsConstants::VISCOCITY_COEFFICIENT * f_viscosity;
			}
		);
	}

	/*
		Update the position in RAM on the CPU side and sends that data to the GPU
	*/
	void FluidSim2D::OnUpdate() 
	{
		static int frame_count = 0;

		if (SimulationConstants::USE_SPATIAL_HASHING) {
			UpdateSpatialHashGrid();
			UpdateParticleDensitySHG();
			UpdateParticlePressure();
			ComputeForcesSHG();
		} else {
			UpdateParticleDensity();
			UpdateParticlePressure();
			ComputeForces();
		}

		std::for_each(std::execution::par_unseq, particles.begin(), particles.end(), 
			[&](Particle& particle) {
				glm::vec2 F_total = particle.F_pressure + 
									particle.F_viscosity + 
									PhysicsConstants::MASS * glm::vec2(0.0f, -PhysicsConstants::GRAVITY);

				particle.acceleration = F_total / PhysicsConstants::MASS;
				particle.velocity += particle.acceleration * GlobalConstants::DT;
				float speed2 = glm::dot(particle.velocity, particle.velocity);
				if (speed2 > SimulationConstants::MaxSpeed() * SimulationConstants::MaxSpeed()) {
					particle.velocity = glm::normalize(particle.velocity) * SimulationConstants::MaxSpeed();
				}
				particle.position += particle.velocity * GlobalConstants::DT;

				// Boundary conditions
				if (particle.position.x < -1.0) {
					particle.position.x = -1.0;
					particle.velocity.x *= SimulationConstants::DAMPENING;
				}

				if (particle.position.x > 1.0) {
					particle.position.x = 1.0;
					particle.velocity.x *= SimulationConstants::DAMPENING;
				}

				if (particle.position.y < -1.0) {
					particle.position.y = -1.0;
					particle.velocity.y *= SimulationConstants::DAMPENING;
				}

				if (particle.position.y > 1.0) {
					particle.position.y = 1.0;
					particle.velocity.y *= SimulationConstants::DAMPENING;
				}
			}
		);

		frame_count++;

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
		float framerate = ImGui::GetIO().Framerate;
		frame_buffer[array_offset] = framerate;

		array_offset = (array_offset + 1) % frame_buffer.size();
		ImGui::PlotLines("FPS",
			frame_buffer.data(),
			frame_buffer.size(),
			array_offset,
			NULL,
			0.0f,  // Min Y-axis
			50.0f, // Max Y-axis
			ImVec2(0, 80.0f) // Graph Size (0 = full width, 80px height)
		);

		ImGui::Text("Applicaton average %.3f ms/frame (%.1f FPS)", 1000.0f / framerate, framerate);
		ImGui::Checkbox("Use Spatial Hashing", &SimulationConstants::USE_SPATIAL_HASHING);
		ImGui::Separator();

		ImGui::SliderFloat("Density (kg/m^2)", &PhysicsConstants::REST_DENSITY, 1.0f, 1415.0f);
		ImGui::SliderFloat("Viscosity (Pa*s)", &PhysicsConstants::VISCOCITY_COEFFICIENT, 0.001f, 0.1f);
		ImGui::SliderFloat("Volume of each drop (m^2)", &PhysicsConstants::MASS, 0.25f, 1.5f);
		ImGui::SliderFloat("Gravity (m/s^2)", &PhysicsConstants::GRAVITY, 1.0f, 25.0f);
		ImGui::SliderFloat("Wall Damping", &SimulationConstants::DAMPENING, -1.0f, 1.0f);
		ImGui::SliderFloat("Smoothing Radius", &PhysicsConstants::SMOOTHING_RADIUS, 0.05f, 3.0f);

	}
}