#pragma once

#include "Simulation.h"

namespace simulation {
	class TestClearColour : public Simulation
	{
	public:
		TestClearColour();
		~TestClearColour();

		void OnUpdate() override;
		void OnRender() override;
		void OnImGuiRender() override;

	private:
		float m_ClearColour[4];
	};
}
