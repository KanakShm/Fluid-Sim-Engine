#include "TestTexture2D.h"

#include "Renderer.h"
#include "imgui/imgui.h"

namespace test {
	TestTexture2D::TestTexture2D()
		: m_ClearColour{ 0.2f, 0.3f, 0.8f, 1.0f }
	{
	}
	TestTexture2D::~TestTexture2D() {}

	void TestTexture2D::OnUpdate(float deltaTime) {}
	void TestTexture2D::OnRender()
	{
		GLCall(glClearColor(0.0f, 0.0f, 0.0f, 1.0f));
		GLCall(glClear(GL_COLOR_BUFFER_BIT));
	}
	void TestTexture2D::OnImGuiRender()
	{
	}
}