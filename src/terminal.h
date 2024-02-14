#pragma once

#include "ansi.h"
#include "element.h"

#include <iostream>

class Terminal
{
  public:
    Terminal()
    {
        ANSIControlCodes::enterRawMode();
        std::cout << ANSIControlCodes::HIDE_CURSOR;
    }
    ~Terminal()
    {
        ANSIControlCodes::restoreTerminalSettings();
        std::cout << ANSIControlCodes::SHOW_CURSOR;
    }

    void render(BaseElement e)
    {
        std::cout << ANSIControlCodes::clearScreen;
        auto size = ANSIControlCodes::getTerminalSize();
        View view{size.cols, size.rows};
        e->render(view);
        std::cout.flush();
    }

    void clear()
    {
        std::cout << ANSIControlCodes::clearScreen;
        std::cout.flush();
    }

    void waitForInput();
};