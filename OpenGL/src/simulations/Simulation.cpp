#include "Simulation.h"
#include "imgui/imgui.h"

namespace simulation {
	SimulationMenu::SimulationMenu(Simulation*& currentTestPointer)
		: m_CurrentTest(currentTestPointer)
	{
	}

	void SimulationMenu::OnImGuiRender()
	{
		for (auto& simulation : m_Simulations)
		{
			if (ImGui::Button(simulation.first.c_str()))
				m_CurrentTest = simulation.second();
		}
	}
}