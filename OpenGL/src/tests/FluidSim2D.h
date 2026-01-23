#pragma once

#include "Test.h"

#include "VertexBuffer.h"
#include "VertexBufferLayout.h"
#include "Texture.h"

#include <memory>

#define NO_OF_PARTICLES 1000000

namespace test {
	class FluidSim2D : public Test
	{
	public:
		struct Particle {
			glm::vec2 position;
			glm::vec2 velocity;
			glm::vec2 acceleration;
			glm::vec3 colour;
		};

		FluidSim2D();
		~FluidSim2D();

		void OnUpdate(float deltaTime) override;
		void OnRender() override;
		void OnImGuiRender() override;

	private:
		std::unique_ptr<VertexArray> m_VAO;
		std::unique_ptr<VertexBuffer> m_VertexBuffer;
		std::unique_ptr<IndexBuffer> m_IndexBuffer;
		std::unique_ptr<Shader> m_Shader;
		std::unique_ptr<Texture> m_Texture;

		glm::mat4 m_Proj, m_View;
		glm::vec3 m_TranslationA, m_TranslationB;

		std::vector<Particle> particles;
		float prev_time;
		int trajectory = 1;
		float capacity = 1.0f;
	};
}
