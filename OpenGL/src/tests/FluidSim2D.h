#pragma once

#include "Test.h"

#include "VertexBuffer.h"
#include "VertexBufferLayout.h"
#include "Texture.h"

#include <memory>
#include <cmath>

#define NO_OF_PARTICLES 1000

namespace PhysicsConstants {
	static constexpr float PI = 3.1415926535f;
	static constexpr float R = 0.05f; // Smoothing radius
	static constexpr float mass = 1.0f;
}

constexpr float calculate_r8(float r) {
	float r2 = r * r;
	float r4 = r2 * r2;
	return r4 * r4;
}

static constexpr float poly6_kernel = 4.0f / (PhysicsConstants::PI * calculate_r8(PhysicsConstants::R));

namespace test {
	class FluidSim2D : public Test
	{
	public:
		struct Particle {
			glm::vec2 position;
			glm::vec2 velocity;
			glm::vec2 acceleration;
			float density;
			float pressure;
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
	};
}
