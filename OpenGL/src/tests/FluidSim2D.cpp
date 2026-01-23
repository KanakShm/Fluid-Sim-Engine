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
			particles{std::vector<Particle>(NO_OF_PARTICLES)}
	{
		// Randomly initialise the position of the particles
		for (int i = 0; i < NO_OF_PARTICLES; ++i) {
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

		m_VertexBuffer = std::make_unique<VertexBuffer>(nullptr, sizeof(Particle) * NO_OF_PARTICLES);
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

	void FluidSim2D::OnUpdate(float curr_time) {
		// Update the position in RAM on the CPU side
		float dt = curr_time - prev_time;
		prev_time = curr_time;
		
		for (auto& pi : particles) {
			float R2 = PhysicsConstants::R * PhysicsConstants::R;
			for (auto& pj : particles) {
				glm::vec2 diff = pj.position - pi.position;
				float x2 = glm::dot(diff, diff);
				if (R2 > x2) {
					float term = R2 - x2;
					pi.density += PhysicsConstants::mass * poly6_kernel * term * term * term;
				}
			}
			pi.position.y += pi.velocity.y * dt;
			if (pi.position.y <= -1.0f) {
				pi.position.y = -0.99f;
				pi.velocity.y *= -1.0f * 0.6f;
			}
			else if (pi.position.y >= 1.0) {
				pi.position.y = 0.99f;
				pi.velocity.y *= -1.0f * 0.6f;
			}
		}

		// Upload the updated vector to the existing GPU buffer
		m_VertexBuffer->Bind();
		GLCall(glBufferSubData(GL_ARRAY_BUFFER, 0, particles.size() * sizeof(Particle), particles.data()));
	}

	void FluidSim2D::OnRender()
	{
		GLCall(glClearColor(0.0f, 0.0f, 0.0f, 1.0f));
		GLCall(glClear(GL_COLOR_BUFFER_BIT));

		Renderer renderer;
		renderer.DrawArraySphere(*m_VAO, *m_Shader, NO_OF_PARTICLES);
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