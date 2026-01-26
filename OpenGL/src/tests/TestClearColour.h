#pragma once

#include "Test.h"

namespace test {
	class TestClearColour : public Test
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
