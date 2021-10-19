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

#pragma once

#include "fontbitmap.h"

#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

class ClockWindow
{
private:
	const int WIDTH = 120, HEIGHT = 50;
	static GLFWwindow* mainWindow;
	static GLuint VBO, VAO, shaderProgram;
	GLFWwindow* window;

	void drawText(const char* text, int posx, int posy, const FontBitmap& font, float scale);
	void getTextExtent(const char* text, const FontBitmap& font, int* width, int* height);
	static void popupMenu(GLFWwindow* window, int button, int action, int mods);

public:
	ClockWindow() = delete;
	ClockWindow(ClockWindow&) = delete;
	ClockWindow(int monitorIdx);

	void initializeGLResources();
	void render(const char* time, const char* date, const FontBitmap& font);

	GLFWwindow* getWindow() const { return window; }
	HWND getHWND() const { return glfwGetWin32Window(window); }
	void makeContextCurrent() const { glfwMakeContextCurrent(window); }
};