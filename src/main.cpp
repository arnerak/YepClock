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

#include <glad/glad.h>
#include <vector>
#include <windows.h>
#include <ctime>

#include "fontbitmap.h"
#include "clockwindow.h"

#include <GLFW/glfw3.h>


int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
    std::vector<ClockWindow*> clockWindows;
    char timeBuffer[32];
    char dateBuffer[32];
    SYSTEMTIME t;
    WORD minuteLastUpdate = -1;

    if (!glfwInit())
        return -1;

    int count;
    GLFWmonitor** monitors = glfwGetMonitors(&count);

    // only one monitor, don't need us
    if (count == 1)
    {
        glfwTerminate();
        return 0;
    }

    // create clocks for all remaining monitors
    for (int i = 1; i < count; i++)
    {
        auto clockWindow = new ClockWindow(i);
        clockWindows.push_back(clockWindow);
    }

    // main window is first in vector, make its context current
    clockWindows.front()->makeContextCurrent();

    // initialize glad
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        return -1;

    FontBitmap font;
    font.create("Segoe UI Variable",9, 0);

    for (auto window : clockWindows)
        window->initializeGLResources();

    // Loop until the user closes the window
    for(;;)
    {
        GetLocalTime(&t);

        // only render the clocks every new minute
        if (t.wMinute != minuteLastUpdate)
        {
            minuteLastUpdate = t.wMinute;

            // use local time and date format settings
            GetTimeFormatA(LOCALE_USER_DEFAULT, TIME_NOSECONDS, &t, NULL, timeBuffer, sizeof(timeBuffer));
            GetDateFormatA(LOCALE_USER_DEFAULT, 0, &t, NULL, dateBuffer, sizeof(dateBuffer));

            for (auto window : clockWindows)
                window->render(timeBuffer, dateBuffer, font);
        }

        for (auto window : clockWindows)
        {
            if (glfwWindowShouldClose(window->getWindow()))
            {
                glfwTerminate();
                return 0;
            }
            // keep the clock windows on top of taskbar
            SetWindowPos(window->getHWND(), HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
        }

        glfwPollEvents();
        Sleep(100);
    }
}
