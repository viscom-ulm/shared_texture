// Copyright 2023 Visual Computing Group, Ulm University
// Author: Jan Eric Haßler

#include "gl_funcs.h"

typedef struct gl_state gl_state;
typedef void (*gl_log_func)(const char *);

typedef struct gl_state
{
    gl_log_func LogFunction;

    shared_texture SharedTexture;
    gl_shared_texture GLSharedTexture;

    GLuint VAO;
    GLuint VBO;
    GLuint Shader;
    GLuint Framebuffer;

    float Time;
} gl_state;

//
//
//

static GLuint OpenGL_CompileShader(gl_state *GL, GLuint Type, const char *Source)
{
    GLuint ShaderID = glCreateShader(Type);
    glShaderSource(ShaderID, 1, &Source, 0);
    glCompileShader(ShaderID);
    GLint ShaderCompiled = GL_FALSE;
    glGetShaderiv(ShaderID, GL_COMPILE_STATUS, &ShaderCompiled);
    if (ShaderCompiled != GL_TRUE)
    {
        char Error[1024] = { 0 };
        glGetShaderInfoLog(ShaderID, 1024, 0, Error);
        glDeleteShader(ShaderID);
        GL->LogFunction(Error);
        return 0;
    }
    return ShaderID;
}

static GLuint OpenGL_LinkProgram(gl_state *GL, GLuint VertexShader, GLuint FragmentShader)
{
    if (!VertexShader) return 0;
    if (!FragmentShader) return 0;

    /* CREATE PROGRAM */
    GLuint ProgramID = glCreateProgram();
    glAttachShader(ProgramID, VertexShader);
    glAttachShader(ProgramID, FragmentShader);

    /* LINK PROGRAM */
    glLinkProgram(ProgramID);
    GLint ProgramSuccess = GL_TRUE;
    glGetProgramiv(ProgramID, GL_LINK_STATUS, &ProgramSuccess);
    if (ProgramSuccess != GL_TRUE)
    {
        char Error[1024] = { 0 };
        glGetProgramInfoLog(ProgramID, 1024, 0, Error);
        glDeleteProgram(ProgramID);
        GL->LogFunction(Error);
        return 0;
    }

    /* CLEAN UP */
    glDetachShader(ProgramID, VertexShader);
    glDetachShader(ProgramID, FragmentShader);
    glDeleteShader(VertexShader);
    glDeleteShader(FragmentShader);

    return ProgramID;
}

static GLuint OpenGL_LoadProgram(gl_state *GL, const char *Vertex, const char *Fragment)
{
    GLuint VertexShader = OpenGL_CompileShader(GL, GL_VERTEX_SHADER, Vertex);
    GLuint FragmentShader = OpenGL_CompileShader(GL, GL_FRAGMENT_SHADER, Fragment);
    return OpenGL_LinkProgram(GL, VertexShader, FragmentShader);
}

static void OpenGL_DebugCallback(GLenum Source, GLenum Type, GLuint Id, GLenum Severity, GLsizei Length,
                          const GLchar *Message, const void *UserParam)
{
    const gl_state *GL = UserParam;
    GL->LogFunction(Message);
}

static bool OpenGL_Init(gl_state *GL, gl_load_function LoadFunction, gl_log_func LogFunction)
{
    if (!GL_LoadFunctions(LoadFunction))
        return false;

    glEnable(GL_FRAMEBUFFER_SRGB);

    GL->LogFunction = LogFunction;
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(OpenGL_DebugCallback, GL);

    GL->SharedTexture = SharedTexture_OpenOrCreate("demo", 1280, 720, SHARED_TEXTURE_RGBA8);
    GL->GLSharedTexture = SharedTexture_ToOpenGL(GL->SharedTexture);

    glGenFramebuffers(1, &GL->Framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, GL->Framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, GL->GLSharedTexture.Texture, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    const GLfloat Vertices[] = {
        0.00f,  0.57f, 1.0f, 0.0f, 0.0f, 1.0f,
       -0.50f, -0.29f, 0.0f, 1.0f, 0.0f, 1.0f,
        0.50f, -0.29f, 0.0f, 0.0f, 1.0f, 1.0f,
    };
    
    glGenVertexArrays(1, &GL->VAO);
    glBindVertexArray(GL->VAO);

    glGenBuffers(1, &GL->VBO);
    glBindBuffer(GL_ARRAY_BUFFER, GL->VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertices), Vertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 6, 0);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 6, (void *)(sizeof(GLfloat) * 2));

    const char *VertexShader = ""
        "#version 450\n"
        "layout(location = 0) in vec2 Position;"
        "layout(location = 1) in vec4 Color;"
        "layout(location = 1) uniform float t;"
        "layout(location = 2) uniform int w;"
        "layout(location = 3) uniform int h;"
        "layout(location = 0) out vec4 vColor;"
        "void main(){"
            "float s = sin(t);"
            "float c = cos(t);"
            "vec2 res = vec2(w, h);"
            "mat2 r = mat2(c, s, -s, c);"
            "gl_Position.xy = r * Position;"
            "gl_Position.x *= res.y / res.x;"
            "gl_Position.zw = vec2(0.0, 1.0);"
            "vColor = Color;"
        "}";

    const char *FragmentShader = ""
        "#version 450\n"
        "layout(location = 0) in vec4 vColor;"
        "out vec4 fColor;"
        "void main(){"
            "fColor = vColor;"
        "}";

    GL->Shader = OpenGL_LoadProgram(GL, VertexShader, FragmentShader);

    return true;
}

static void OpenGL_Update(gl_state *GL, int32_t Width, int32_t Height, float Delta)
{
    GL->Time += Delta;

    if (SharedTexture_OpenGLWait(GL->GLSharedTexture))
    {
        // DRAW TO SHARED TEXTURE
        glViewport(0, 0, GL->SharedTexture.Width, GL->SharedTexture.Height);
        glBindFramebuffer(GL_FRAMEBUFFER, GL->Framebuffer);
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(GL->Shader);
        glBindVertexArray(GL->VAO);
        glUniform1f(1, GL->Time);
        glUniform1i(2, GL->SharedTexture.Width);
        glUniform1i(3, GL->SharedTexture.Height);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        SharedTexture_OpenGLSignal(GL->GLSharedTexture);
    }

    // BLIT TO OPENGL WINDOW
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, GL->Framebuffer);
    glBlitFramebuffer(0, 0, GL->SharedTexture.Width, GL->SharedTexture.Height,
                      0, 0, Width, Height,
                      GL_COLOR_BUFFER_BIT, GL_NEAREST);

}
