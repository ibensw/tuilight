#pragma once

#include "ansi.h"
#include "element.h"

#include <atomic>
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
        std::cout << ANSIControlCodes::RESET << ANSIControlCodes::SHOW_CURSOR;
    }

    void render(BaseElement e)
    {
        std::cout << ANSIControlCodes::clearScreen;
        auto size = ANSIControlCodes::getTerminalSize();
        TerminalView view{size.cols, size.rows};
        e->render(view);
        std::cout.flush();
    }

    void clear()
    {
        std::cout << ANSIControlCodes::clearScreen;
        std::cout.flush();
    }

    void runInteractive(BaseElement e)
    {
        running = true;
        if (e->focusable()) {
            e->setFocus(true);
        }
        while (running) {
            clear();
            render(e);
            auto key = getchar();
            if (key == 27) {
                running = false;
            }
            e->handleEvent(key);
        }
    }

    void stop() { running = false; }

    void waitForInput();

  private:
    std::atomic<bool> running;
};