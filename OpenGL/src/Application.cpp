#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <iostream>

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

/*
First we get an id from the GPU that corresponds to the current shader needing to be created. It needs the type of shader
to be created (for eg. GL_VERTEX_SHADER, GL_FRAGMENT_SHADER). Next, we link the id (more precicely the actual shader
information on the gpu) to an actual c style string. The second elemnt specifies the number of elements in the string and
length arrays. The third argument of the function specifies and array of pointers to strings containing the source code to
be loaded on the shader. The final argument specifies the length of each string. Compile shader actually compiles the shader.
*/

int main(void)
{
    GLFWwindow* window;
   
    /* Initialize the library */
    if (!glfwInit())
        return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    /* Create a windowed mode window and its OpenGL context */
    window = glfwCreateWindow(800, 600, "Hello World", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }

    /* Make the window's context current */
    glfwMakeContextCurrent(window);

    if (glewInit() != GLEW_OK)
        std::cout << "Error!" << std::endl;
    glEnable(GL_DEPTH_TEST);
    std::cout << glGetString(GL_VERSION) << std::endl;

    /*
        First we generate a unique id that is assigned to an unsigned int. This id is passed to the GPU driver and is associated
        with GL_ARRAY_BUFFER, which means that it is the current 'target'. Next, we allocate space on the GPU. GL_ARRAY_BUFFER is
        the target that we'll be allocating for, 6 * sizeof(float) is how big the data is, 0 means don't upload any data yet
        and the last flag clues the GPU into how the buffer is going to be used. GL_STATIC_DRAW is if the data on the buffer is not going to 
        change, GL_DYNAMIC_DRAW is if it is going to change, GL_STREAM_DRAW is if it is going to change every loop or very frequently. Then we 
        can actually send the data by using glBufferSubData(...) to the GPU. Right now that data is just there existing and isnt doing anything 
        just yet.
    */

    {
        float vertices[] = {
            -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
             0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
             0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
             0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
            -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
            -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,

            -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
             0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
             0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
             0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
            -0.5f,  0.5f,  0.5f,  0.0f, 1.0f,
            -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,

            -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
            -0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
            -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
            -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
            -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
            -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

             0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
             0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
             0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
             0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
             0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
             0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

            -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
             0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
             0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
             0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
            -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
            -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,

            -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
             0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
             0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
             0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
            -0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
            -0.5f,  0.5f, -0.5f,  0.0f, 1.0f
        };

        unsigned int indices[] = {
            0, 1, 2, 3, 4, 5, 6,
            7, 8, 9, 10, 11, 12,
            13, 14, 15, 16, 17, 18,
            19, 20, 21, 22, 23, 24,
            25, 26, 27, 28, 29, 30,
            31, 32, 33, 34, 35,
        };

        glm::vec3 cubePositions[] = {
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(-1.2f, -1.2f, 1.5f),
            glm::vec3(0.6f, 0.5f, -2.5f),
            glm::vec3(2.6f, -0.5f, -3.0f),
            glm::vec3(1.0f, -1.0f, -4.0f),
            glm::vec3(-1.4f, -2.0f, -5.0f),
            glm::vec3(-3.6f, -6.5f, -5.0f),
            glm::vec3(2.3f, 1.2f, 0.5f),
            glm::vec3(-2.3f, 1.2f, -2.5f),
        };

        GLCall(glEnable(GL_BLEND));
        GLCall(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

        // Initialising pointers to vertex buffer object (memory buffer on the gpu that 
        // holds the list of x,y,z coordinates), vertex array object (records which VBO is being used
        // and how vertexAttribPointer is configured) and element buffer object (list of unique coordinates).

        // Create va and vb and bind vb to the active GL_ARRAY_BUFFER
        VertexArray va;
        VertexBuffer vb(vertices, sizeof(vertices));

        // Create a layout for the data and call its respective attrib pointers
        VertexBufferLayout layout;
        layout.Push<float>(3);
        layout.Push<float>(2);
        va.AddBuffer(vb, layout);

        IndexBuffer ib(indices, sizeof(indices));
        va.Bind();  // Binding it again i dont know why its needed
        ib.Bind();

        Shader shader("res/shaders/Basic.shader");
        shader.Bind();

        /*
        Here we start by creating an unsigned int called texture that acts as the name/id of the current texture being
        activated. glActiveTexture associates the current texture to be worked on? (Not clear on this). Then we use glBindTexture to
        set the recently created texture to the current global active 2D texture (Does this mean that only one 2D texture can be
        created/used at a time since there can only be one global active one?). We then load the image texture, convert it to pixel
        format and upload it to the GPU's currently active texture? Mimmap is autocreated from that function we call at the end.
        */
        // Load in and create the texture.
        Texture texture1("res/textures/container.jpg");
        texture1.Bind(0);
        shader.SetUniform1i("u_Texture1", 0);
        

        Texture texture2("res/textures/awesomeface.png");
        texture2.Bind(1);
        shader.SetUniform1i("u_Texture2", 1);

        float blend = 0.0f;
        float increment = 0.05f;

        Renderer renderer;

        ImGui::CreateContext();
        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init("#version 330");
        ImGui::StyleColorsDark();

        glfwSwapInterval(2);
        glm::vec3 translation(0, 0, 0);
        /* Loop until the user closes the window */
        while (!glfwWindowShouldClose(window))
        {
            // render
            // clear the colour buffer
            renderer.Clear();

            // Start GUI frame
            ImGui_ImplOpenGL3_NewFrame();  // Start the renderer
            ImGui_ImplGlfw_NewFrame();     // Start the platform (Tells ImGui where the mouse/keyboard is)
            ImGui::NewFrame();             // Start the logic

            // Render the container
            texture1.Bind(0);
            texture2.Bind(1);

            glm::mat4 proj = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);
            glm::mat4 view = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -6.0f));

            float time = glfwGetTime();
            
            if (blend > 1.0) increment = -0.005f;
            if (blend < 0.0) increment = 0.005f;
            blend += increment;
            for (unsigned int i = 0; i < 9; i++) {;
                glm::mat4 model = glm::translate(glm::mat4(1.0), cubePositions[i] + translation);
                glm::mat4 trans = glm::rotate(glm::mat4(1.0f), glm::radians(float(std::pow(-1, i)) * time * 35), glm::vec3(float(std::pow(-1,i) - 21), float(std::pow(-1, i) + 40), float(std::pow(-1, i) + 2)));

                glm::mat4 mvp = proj * view * model * trans;
                shader.SetUniformMat4f("u_MVP", mvp);
                shader.SetUniform1f("blend", blend);
                renderer.Draw(va, ib, shader);
            }
            {
                static float f = 0.0f;
                static int counter = 0;
                ImGui::Begin("Hello World");
                ImGui::SliderFloat3("Translation", &translation.x, -10.0f, 10.0f);
                ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
                ImGui::End();
            }


            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            /* Swap front and back buffers */
            GLCall(glfwSwapBuffers(window));

            /* Poll for and process events */
            GLCall(glfwPollEvents());
        }
        shader.Unbind();
        vb.Unbind();
        ib.Unbind();
        va.Unbind();
    }
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    GLCall(glfwTerminate());
    return 0;
}    