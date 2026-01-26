#pragma once

#include "Test.h"

#include "VertexBuffer.h"
#include "VertexBufferLayout.h"
#include "Texture.h"

#include <memory>
#include <cmath>
#include <array>
#include <algorithm>
#include <execution>

constexpr float calculate_r6(float r) {
	float r2 = r * r;
	return r2 * r2 * r2;
}

constexpr float calculate_r8(float r) {
	float r2 = r * r;
	float r4 = r2 * r2;
	return r4 * r4;
}

namespace PhysicsConstants {
	static constexpr float PI = 3.1415926535f;

	static float SMOOTHING_RADIUS = 0.16f;
	static float MASS = 0.00005;
	static float REST_DENSITY = 1000.0f;
	static float VISCOCITY_COEFFICIENT = 0.01;
	static float GASS_CONSTANT = 0.420f;
	static float GRAVITY = 9.81f;
	static float Poly6Kernal() {
		return 4.0f / (PhysicsConstants::PI * calculate_r8(PhysicsConstants::SMOOTHING_RADIUS));
	}
	static float SpikeyConstant() {
		return -45.0f / (PhysicsConstants::PI * calculate_r6(PhysicsConstants::SMOOTHING_RADIUS));
	}
	static float MullerConstant() {
		return 45.0f / (PhysicsConstants::PI * calculate_r6(PhysicsConstants::SMOOTHING_RADIUS));
	}
}

namespace SimulationConstants {
	static constexpr int NO_OF_PARTICLES = 2000;
	static constexpr int TABLE_SIZE = NO_OF_PARTICLES * 2;
	static constexpr int PRIME1 = 98561123;
	static constexpr int PRIME2 = 863421509;
	static constexpr float SAFETY_FACTOR = 0.4;

	static float DAMPENING = -0.3f;
	static bool USE_SPATIAL_HASHING = true;
	static float MaxSpeed() {
		return PhysicsConstants::SMOOTHING_RADIUS* SAFETY_FACTOR / GlobalConstants::DT;
	}
}

namespace Init {
	static constexpr float START_X = -0.5f;
	static constexpr float START_Y = -0.0f;
	static constexpr float SPACING_X = 1.5f;
	static constexpr float SPACING_Y = 1.5f;
	static constexpr int PPR = 100.0f;
}

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
			glm::vec2 F_pressure;
			glm::vec2 F_viscosity;
			glm::vec2 F_other;
			glm::vec3 colour;
		};

		FluidSim2D();
		~FluidSim2D();

		void UpdateSpatialHashGrid();
		void UpdateParticleDensity();
		void UpdateParticlePressure();

		void ComputeForces();

		void OnUpdate() override;
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

		std::vector<std::array<int, 2>> spatialHash =
			std::vector<std::array<int, 2>>(SimulationConstants::NO_OF_PARTICLES, {INT_MAX, INT_MAX});

		std::vector<int> indices =
			std::vector<int>(SimulationConstants::TABLE_SIZE, -1);

		std::vector<int> iter_idx = 
			[]() {
				std::vector<int> v(SimulationConstants::NO_OF_PARTICLES);
				std::iota(v.begin(), v.end(), 0);
				return v;
			}();

	};
}
