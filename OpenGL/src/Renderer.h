#pragma once

#include <GL/glew.h>
#include <signal.h>

#include "VertexArray.h"
#include "IndexBuffer.h"
#include "Shader.h"

#ifdef _MSC_VER
    #define DEBUG_BREAK() __debugbreak();
#elif defined(__EMSCRIPTEN__)
    #include <stdlib.h>
    #define DEBUG_BREAK() abort();
#else
    #define DEBUG_BREAK() __builtin_trap();
#endif

#define ASSERT(x) if (!(x)) DEBUG_BREAK();
#define GLCall(x) GLClearError();\
    x;\
    ASSERT(GLLogCall(#x, __FILE__, __LINE__))

void GLClearError();
bool GLLogCall(const char* function, const char* file, int line);

class Renderer {
public:
    void DrawElementTriangle(const VertexArray& va, const IndexBuffer& ib, const Shader& shader) const;
    void DrawArraySphere(const VertexArray& va, const Shader& shader, const int& count) const;
    void Clear() const;
};