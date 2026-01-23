#include "FluidSim2D.h"

#include "Renderer.h"
#include "imgui/imgui.h"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
namespace test {
	FluidSim2D::FluidSim2D()
		: m_Proj(glm::ortho(0.0f, 960.0f, 0.0f, 540.0f, -1.0f, 1.0f)), 
			m_View(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f))), 
			m_TranslationA(200, 200, 0), m_TranslationB(400, 200, 0), 
			particles{std::vector<Particle>(NO_OF_PARTICLES)}
	{
		// Randomly initialise the position of the particles
		for (int i = 0; i < NO_OF_PARTICLES; ++i) {
			particles[i].position.x = (float)(std::rand()) / RAND_MAX - 0.5f;
			particles[i].position.y = (float)(std::rand()) / RAND_MAX - 0.5f;

			particles[i].velocity.x = 0.0;
			particles[i].velocity.y = 0.0;

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
		layout.Push<float>(2);
		layout.Push<float>(2);
		layout.Push<float>(2);
		layout.Push<float>(3);

		m_VAO->AddBuffer(*m_VertexBuffer, layout);
		m_Shader->Bind();
	}
	FluidSim2D::~FluidSim2D() {}

	void FluidSim2D::OnUpdate(float curr_time) {
		// Update the position in RAM on the CPU side
		for (auto& particle : particles) {
			particle.position.y -= particle.velocity.y * (curr_time - prev_time);
			if (particle.position.y <= -1.0 || particle.position.y >= 1.0) {
				particle.velocity.y *= -1;
			}
			prev_time = curr_time;
		}

		prev_time = curr_time;

		// Upload the updated vector to the existing GPU buffer
		m_VertexBuffer->Bind();
		glBufferData(GL_ARRAY_BUFFER, particles.size() * sizeof(Particle), nullptr, GL_DYNAMIC_DRAW);
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