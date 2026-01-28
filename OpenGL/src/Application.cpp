#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <iostream>

#ifdef __EMSCRIPTEN__
    #include <emscripten.h>
#endif

#include "Renderer.h"

#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "VertexArray.h"
#include "VertexBufferLayout.h"
#include "Shader.h"
#include "Texture.h"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

#include "simulations/TestClearColour.h"
#include "simulations/TestTexture2D.h"
#include "simulations/FluidSim2D.h"

struct AppState {
    GLFWwindow* window;
    Renderer* renderer;
    simulation::Simulation* currentSimulation;
    simulation::SimulationMenu* simulationMenu;
    float accumulator;
    float last_time;
};

void MainLoop(void* arg)
{
    AppState* state = (AppState*)arg;

    GLCall(glClearColor(0.0f, 0.0f, 0.0f, 1.0f));
    state->renderer->Clear();

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    if (state->currentSimulation)
    {
        float curr_time = glfwGetTime();
        float frame_time = curr_time - state->last_time;
        state->last_time = curr_time;

        if (frame_time > 0.25f) frame_time = 0.25f;

        state->accumulator += frame_time;
        int steps = 0;
        int MAX_STEPS = 5;

        // Physics Update
        while (state->accumulator >= GlobalConstants::DT && steps < GlobalConstants::DT)
        {
            state->currentSimulation->OnUpdate();
            state->accumulator -= GlobalConstants::DT;
            steps++;
        }
        if (steps >= MAX_STEPS) {
            state->accumulator = 0.0f;
        }

        // Render
        state->currentSimulation->OnRender();

        // ImGui Logic
        #ifdef __EMSCRIPTEN__
            ImGui::SetNextWindowSize(ImVec2(400, 370), ImGuiCond_Always);
        #endif
        ImGui::Begin("Fluid Simulation");

        if (state->currentSimulation != state->simulationMenu && ImGui::Button("<-"))
        {
            delete state->currentSimulation;
            state->currentSimulation = state->simulationMenu;
        }
        state->currentSimulation->OnImGuiRender();
        ImGui::End();
    }

    // Finalize ImGui
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // Swap Buffers
    glfwSwapBuffers(state->window);
    glfwPollEvents();
}

int main(void)
{
    GLFWwindow* window;
    int windowWidth = GlobalConstants::WINDOW_WIDTH;
    int windowHeight = GlobalConstants::WINDOW_HEIGHT;
    #ifdef __EMSCRIPTEN__
        int browserWidth = emscripten_run_script_int("window.innerWidth");
        int browserHeight = emscripten_run_script_int("window.innerHeight");

        windowWidth = (int)(browserWidth * 0.90);
        windowHeight = (int)(browserHeight * 0.90);
    #endif
   
    /* Initialize the library */
    if (!glfwInit())
        return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    /* Create a windowed mode window and its OpenGL context */
    window = glfwCreateWindow(GlobalConstants::WINDOW_WIDTH, GlobalConstants::WINDOW_HEIGHT, "Fluid Simulation Engine", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }

    /* Make the window's context current */
    glfwMakeContextCurrent(window);
    
    #ifndef __EMSCRIPTEN__
        if (glewInit() != GLEW_OK)
            std::cout << "Error!" << std::endl;
    #endif

    #ifndef __EMSCRIPTEN__
        glEnable(GL_PROGRAM_POINT_SIZE);
    #endif
    
    glEnable(GL_DEPTH_TEST);
    std::cout << glGetString(GL_VERSION) << std::endl;

    {
        GLCall(glEnable(GL_BLEND));
        GLCall(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

        // --- SETUP APP STATE ---
        AppState app;
        app.window = window;
        app.renderer = new Renderer();
        app.accumulator = 0.0f;
        app.last_time = glfwGetTime();

        // Setup ImGui
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.IniFilename = NULL;
        ImGui_ImplGlfw_InitForOpenGL(window, true);

        const char* glsl_version = "#version 330";
        #ifdef __EMSCRIPTEN__
            glsl_version = "#version 300 es";
        #endif
        ImGui_ImplOpenGL3_Init(glsl_version);

        ImGui::StyleColorsDark();

        // Setup Menu
        app.currentSimulation = nullptr;
        app.simulationMenu = new simulation::SimulationMenu(app.currentSimulation);
        app.currentSimulation = app.simulationMenu;

        app.simulationMenu->RegisterSimulation<simulation::FluidSim2D>("Fluid Simulation");

        // --- THE MAIN LOOP SWITCH ---
        #ifdef __EMSCRIPTEN__
            // 0 = 0 FPS (uses RequestAnimationFrame, usually 60fps)
            // 1 = simulate infinite loop (prevents code below from running immediately)
            emscripten_set_main_loop_arg(MainLoop, &app, 0, 1);
        #else

        // DESKTOP: Standard loop
        while (!glfwWindowShouldClose(window))
        {
            MainLoop(&app);
        }
        #endif

        // Cleanup (Only reached on Desktop or when tab closes)
        // On Web, this part is technically never reached in the same way, 
        // but the browser handles memory cleanup on tab close.
        if (app.currentSimulation != app.simulationMenu)
            delete app.currentSimulation;
        delete app.simulationMenu;
        delete app.renderer;
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    GLCall(glfwTerminate());
    return 0;
}