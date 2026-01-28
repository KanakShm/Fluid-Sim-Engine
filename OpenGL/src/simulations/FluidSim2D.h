#pragma once

#include "Simulation.h"

#include "VertexBuffer.h"
#include "VertexBufferLayout.h"
#include "Texture.h"

#include <memory>
#include <cmath>
#include <array>
#include <algorithm>
#include <numeric>

#ifndef __EMSCRIPTEN__
	#include <execution>
#endif

namespace Utils {
	template <typename Iterator, typename T>
	void ParallelFill(Iterator begin, Iterator end, const T& val)
	{
#ifdef __EMSCRIPTEN__
		std::fill(begin, end, val);
#else
		std::fill(std::execution::par_unseq, begin, end, val);
#endif
	}

	template <typename Iterator, typename Func>
	void ParallelForEach(Iterator begin, Iterator end, Func func)
	{
#ifdef __EMSCRIPTEN__
		std::for_each(begin, end, func);
#else
		std::for_each(std::execution::par_unseq, begin, end, func);
#endif
	}

	template <typename Iterator, typename Compare>
	void ParallelSort(Iterator begin, Iterator end, Compare comp)
	{
#ifdef __EMSCRIPTEN__
		std::sort(begin, end, comp);
#else
		std::sort(std::execution::par_unseq, begin, end, comp);
#endif
	}
}

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

	inline static float SMOOTHING_RADIUS = 0.16f;
	inline static float MASS = 1.0f;
	inline static float REST_DENSITY = 1415.0f;
	inline static float VISCOCITY_COEFFICIENT = 0.016;
	inline static float GASS_CONSTANT = 0.420f;
	inline static float GRAVITY = 9.81f;
	inline static float Poly6Kernal() {
		return 4.0f / (PhysicsConstants::PI * calculate_r8(PhysicsConstants::SMOOTHING_RADIUS));
	}
	inline static float SpikeyConstant() {
		return -45.0f / (PhysicsConstants::PI * calculate_r6(PhysicsConstants::SMOOTHING_RADIUS));
	}
	inline static float MullerConstant() {
		return 45.0f / (PhysicsConstants::PI * calculate_r6(PhysicsConstants::SMOOTHING_RADIUS));
	}
}

namespace SimulationConstants {
#if defined(__EMSCRIPTEN__)
	static constexpr int NO_OF_PARTICLES = 2500;
#else
	static constexpr int NO_OF_PARTICLES = 2500;
#endif
	static constexpr int TABLE_SIZE = NO_OF_PARTICLES * 2;
	static constexpr int PRIME1 = 98561123;
	static constexpr int PRIME2 = 863421509;
	static constexpr float SAFETY_FACTOR = 0.40f;

	inline static float DAMPENING = -0.3f;
	inline static float GRAB_RADIUS = 0.3f;
	inline static float GRAB_STRENGTH = -12000.0f;
	inline static bool USE_SPATIAL_HASHING = true;
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

namespace simulation {
	class FluidSim2D : public Simulation
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

		void ResetForces();
		glm::vec2 GetMouseWorldPos();
		void HandleMouseInteraction();

		void UpdateSpatialHashGrid();
		void UpdateParticleDensity();
		void UpdateParticleDensitySHG();

		void UpdateParticlePressure();

		void ComputeForces();
		void ComputeForcesSHG();

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
		
		std::array<float, 90> frame_buffer = {};
		int array_offset = 0;

	};
}