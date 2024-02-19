#pragma once

#include "element.h"
#include <atomic>
#include <functional>
#include <list>

class Terminal : public View
{
  public:
    Terminal();
    ~Terminal();

    void render(BaseElement e);
    void clear();
    void runInteractive(BaseElement e);
    void stop() { running = false; }

    KeyEvent keyPress();

    void write(std::size_t column, std::size_t row, Style style, std::string_view data) override;
    void printStyle(const Style &style);

    void post(std::function<void(Terminal &, BaseElement)> fun);
    void postKeyPress(KeyEvent event);

  private:
    std::atomic<bool> running;
    std::list<std::function<void(Terminal &, BaseElement)>> callbacks; // No need for locks
    int pipeFd[2];
};
