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
		layout.Push<float>(3);	// Colour

		m_VAO->AddBuffer(*m_VertexBuffer, layout);
		m_Shader->Bind();
	}
	FluidSim2D::~FluidSim2D() {}

	// Update the position in RAM on the CPU side
	void FluidSim2D::OnUpdate(float curr_time) {
		float dt = curr_time - prev_time;
		prev_time = curr_time;

		const int TABLE_SIZE = SimulationConstants::NO_OF_PARTICLES * 2;
		std::vector<int> iter_idx(SimulationConstants::NO_OF_PARTICLES);
		std::iota(iter_idx.begin(), iter_idx.end(), 0);

		// Initialise and fill the spatial hash table
		std::vector<std::array<int, 2>> spatialHash(SimulationConstants::NO_OF_PARTICLES, std::array<int, 2>{INT_MAX, INT_MAX});
		std::for_each(std::execution::par_unseq, iter_idx.begin(), iter_idx.end(), 
			[&](int i) {
				int coord_x = std::floor((particles[i].position.x + 1) / PhysicsConstants::R);
				int coord_y = std::floor((particles[i].position.y + 1) / PhysicsConstants::R);
				int grid_hash = (coord_x * SimulationConstants::PRIME1) ^ (coord_y * SimulationConstants::PRIME2);
				grid_hash = std::abs(grid_hash) % TABLE_SIZE;

				spatialHash[i] = { grid_hash, i };
			}
		);

		std::sort(std::execution::par_unseq, spatialHash.begin(), spatialHash.end(), std::less<std::array<int, 2>>());

		// Initialise and fill the indice lookup table
		std::vector<int> indices(TABLE_SIZE, -1);
		std::for_each(std::execution::par_unseq, iter_idx.begin(), iter_idx.end(),
			[&](int i) {
				int prev_hash = i == 0 ? -1 : spatialHash[i - 1][0];

				if (spatialHash[i][0] != INT_MAX &&
					spatialHash[i][0] != prev_hash) {
					indices[spatialHash[i][0]] = i;
				}
			}
		);

		// Iterate through all particles and calculate density
		float R2 = PhysicsConstants::R * PhysicsConstants::R;
		int grid_size = std::floor(2.0f / PhysicsConstants::R);

		std::for_each(std::execution::par_unseq, iter_idx.begin(), iter_idx.end(),
			[&](int i) {
				Particle& particle = particles[i];
				particle.density = 0.0f;

				// get the grid coordinate of that particle
				int coord_x = std::floor((particle.position.x + 1) / PhysicsConstants::R);
				int coord_y = std::floor((particle.position.y + 1) / PhysicsConstants::R);

				// get surrounding grid coordinates
				for (int j = -1; j <= 1; ++j) {
					for (int k = -1; k <= 1; ++k) {
						int target_x = coord_x + j;
						int target_y = coord_y + k;

						int target_grid_hash = (target_x * SimulationConstants::PRIME1) ^ (target_y * SimulationConstants::PRIME2);
						target_grid_hash = std::abs(target_grid_hash) % TABLE_SIZE;

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
		/*Renderer renderer;

		m_Texture->Bind();
		m_Proj = glm::ortho(0.0f, 960.0f, 0.0f, 540.0f, -1.0f, 1.0f);
		m_View = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));

		{
			glm::mat4 model = glm::translate(glm::mat4(1.0f), m_TranslationA);
			glm::mat4 mvp = m_Proj * m_View * model;
			m_Shader->Bind();
			m_Shader->SetUniformMat4f("u_MVP", mvp);
			renderer.Draw(*m_VAO, *m_IndexBuffer, *m_Shader);
		}
		{
			glm::mat4 model = glm::translate(glm::mat4(1.0f), m_TranslationB);
			glm::mat4 mvp = m_Proj * m_View * model;
			m_Shader->Bind();
			m_Shader->SetUniformMat4f("u_MVP", mvp);
			renderer.Draw(*m_VAO, *m_IndexBuffer, *m_Shader);
		}*/
	}

	void FluidSim2D::OnImGuiRender()
	{
		ImGui::Text("Applicaton average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
	}
}