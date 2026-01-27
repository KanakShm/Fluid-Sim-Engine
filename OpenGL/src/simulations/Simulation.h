#pragma once

#include <functional>
#include <string>
#include <iostream>

namespace GlobalConstants {
	static constexpr int WINDOW_HEIGHT = 900;
	static constexpr int WINDOW_WIDTH = 1600;
	static constexpr float DT = 0.005;
}

namespace simulation {
	class Simulation
	{
	public:
		Simulation() {}
		virtual ~Simulation() {}

		virtual void OnUpdate() {}
		virtual void OnRender() {}
		virtual void OnImGuiRender() {}
	};

	class SimulationMenu : public Simulation
	{
	public:
		SimulationMenu(Simulation*& currentTestPointer);

		void OnImGuiRender() override;

		template<typename T>
		void RegisterSimulation(const std::string& name)
		{
			std::cout << "Registering simulation " << name << std::endl;

			m_Simulations.push_back(std::make_pair(name, []() { return new T(); }));
		}
	private:
		Simulation*& m_CurrentTest;
		std::vector<std::pair<std::string, std::function<Simulation* ()>>> m_Simulations;
	};

}
