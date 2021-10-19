/*
Copyright (c) 2021 Arne Rak

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would
   be appreciated but is not required.

2. Altered source versions must be plainly marked as such, and must not
   be misrepresented as being the original software.

3. This notice may not be removed or altered from any source
   distribution.
*/

#include <windows.h>
#include <glad/glad.h>
#include "clockwindow.h"

/*********************************************/
/**************** Shader code ****************/
/*********************************************/
const char* vertShader = "\
#version 330 core\n\
layout (location = 0) in vec4 vertex; \
out vec2 TexCoords;\
uniform mat4 projection;\
void main()\
{\
    gl_Position = projection * vec4(vertex.xy, 0.0, 1.0);\
    TexCoords = vertex.zw;\
}";

const char* fragShader = "\
#version 330 core\n\
in vec2 TexCoords;\
uniform sampler2D text;\
void main()\
{\
    gl_FragColor = vec4(1.0, 1.0, 1.0, texture2D(text, TexCoords).r);\
}";
/*********************************************/

GLFWwindow* ClockWindow::mainWindow = NULL;
GLuint ClockWindow::VBO = -1, ClockWindow::VAO = -1, ClockWindow::shaderProgram = -1;

ClockWindow::ClockWindow(int monitorIdx)
{
    int count, monitorX, monitorY;

    GLFWmonitor** monitors = glfwGetMonitors(&count);
    const GLFWvidmode* videoMode = glfwGetVideoMode(monitors[monitorIdx]);

    glfwGetMonitorPos(monitors[monitorIdx], &monitorX, &monitorY);

    // compute the taskbar height to account for different scaling settings
    int waxpos, waypos, wawidth, waheight;
    glfwGetMonitorWorkarea(monitors[monitorIdx], &waxpos, &waypos, &wawidth, &waheight);
    int taskbarHeight = videoMode->height - waheight;

    glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);
    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
    glfwWindowHint(GLFW_FLOATING, GLFW_TRUE);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_FALSE);

    window = glfwCreateWindow(WIDTH, HEIGHT, "", NULL, mainWindow);

    // first window created will share its resources with future windows
    if (mainWindow == NULL)
        mainWindow = window;

    // make the created window a toolwindow to hide its taskbar icon
    SetWindowLongPtr(glfwGetWin32Window(window), GWL_EXSTYLE, WS_EX_TOOLWINDOW | WS_EX_TOPMOST);

    // vertically center window on top of taskbar
    glfwSetWindowPos(window,
        monitorX + videoMode->width - WIDTH - 10,
        monitorY + videoMode->height - (HEIGHT + taskbarHeight) / 2);

    glfwShowWindow(window);

    glfwSetWindowSize(window, WIDTH, HEIGHT);

    // set up popup menu
    glfwSetMouseButtonCallback(window, popupMenu);
}

void ClockWindow::drawText(const char* text, int posx, int posy, const FontBitmap& font, float scale)
{
    const float w = font.getGlyphWidth() * scale;
    const float h = font.getGlyphHeight() * scale;
    const float uw = font.getUW();
    const float vh = font.getVH();

    int textExtentX, textExtentY;
    getTextExtent(text, font, &textExtentX, &textExtentY);
    // align right
    int xoffset = -textExtentX;

    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(VAO);

    glBindTexture(GL_TEXTURE_2D, font.getGLTexture());
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    int i = -1;
    int currentStrWidth = 0;
    while (char currentChar = text[++i])
    {
        const Glyph& glyph = font.getGlyph(currentChar);
        const float u = glyph.u;
        const float v = glyph.v;
        const float x = posx + (currentStrWidth + xoffset + glyph.xoff) * scale;
        const float y = posy - glyph.yoff * scale;
        currentStrWidth += glyph.advance;

        float vertices[6][4]
        {
            { x,		y,		u, v + vh },
            { x + w,	y,		u + uw, v + vh },
            { x,		y + h,	u, v },
            { x + w,	y,		u + uw, v + vh },
            { x + w,	y + h,	u + uw, v },
            { x,		y + h,	u, v }
        };

        // copy glyph vertices into VBO
        glBufferSubData(GL_ARRAY_BUFFER, i * sizeof(vertices), sizeof(vertices), vertices);
    }

    // render text
    glDrawArrays(GL_TRIANGLES, 0, i * 6);

    // unbind resources
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void ClockWindow::getTextExtent(const char* text, const FontBitmap& font, int* width, int* height)
{
    auto len = strlen(text);
    int w = 0;
    for (int i = 0; i < len-1; i++)
        w += font.getGlyph(text[i]).advance;

    // for last character, use character width instead of advance
    w += font.getGlyph(text[len-1]).charWidth;

    *width = w;
    *height = font.getFontHeightAboveBaseline();
}

void ClockWindow::initializeGLResources()
{
    makeContextCurrent();

    // since resources are shared between windows, we only need to create them once
    if (window == mainWindow)
    {
        // compile shaders and link program
        GLuint vertId, fragId;
        vertId = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertId, 1, &vertShader, 0);
        glCompileShader(vertId);

        fragId = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragId, 1, &fragShader, 0);
        glCompileShader(fragId);

        shaderProgram = glCreateProgram();
        glAttachShader(shaderProgram, vertId);
        glAttachShader(shaderProgram, fragId);
        glLinkProgram(shaderProgram);

        glDeleteShader(vertId);
        glDeleteShader(fragId);

        // generate ortho projection matrix
        float FarZ = 1000.f;
        float NearZ = 0.1f;
        float ReciprocalWidth = 1.0f / WIDTH;
        float ReciprocalHeight = 1.0f / HEIGHT;
        float fRange = 1.0f / (FarZ - NearZ);
        float proj[4][4];
        proj[0][0] = ReciprocalWidth + ReciprocalWidth;
        proj[0][1] = 0.0f;
        proj[0][2] = 0.0f;
        proj[0][3] = 0.0f;
        proj[1][0] = 0.0f;
        proj[1][1] = ReciprocalHeight + ReciprocalHeight;
        proj[1][2] = 0.0f;
        proj[1][3] = 0.0f;
        proj[2][0] = 0.0f;
        proj[2][1] = 0.0f;
        proj[2][2] = fRange;
        proj[2][3] = 0.0f;
        proj[3][0] = -1;
        proj[3][1] = -1;
        proj[3][2] = -fRange * NearZ;
        proj[3][3] = 1.0f;

        // set projection uniform
        glUseProgram(shaderProgram);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, &proj[0][0]);

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4 * 100, NULL, GL_DYNAMIC_DRAW);
    }

    glEnable(GL_TEXTURE_2D);
    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void ClockWindow::popupMenu(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
    {
        auto menu = CreatePopupMenu();
        UINT_PTR ID_CLOSE = 1;
        InsertMenuA(menu, 0, MF_BYPOSITION | MF_STRING, ID_CLOSE, "Close");

        POINT pos;
        GetCursorPos(&pos);

        auto selection = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_CENTERALIGN | TPM_BOTTOMALIGN, pos.x, pos.y, 0, glfwGetWin32Window(window), NULL);
        if (selection == ID_CLOSE)
        {
            glfwSetWindowShouldClose(window, 1);
        }
    }
}

void ClockWindow::render(const char* time, const char* date, const FontBitmap& font)
{
    makeContextCurrent();

    // render
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // account for different scaling settings
    float xscale, yscale;
    glfwGetWindowContentScale(window, &xscale, &yscale);

    glUseProgram(shaderProgram);
    drawText(time, WIDTH - 10, HEIGHT / 2, font, xscale);
    drawText(date, WIDTH - 10, HEIGHT / 2 - (font.getGlyphHeight() + 2) * xscale, font, xscale);

    glFlush();
}
