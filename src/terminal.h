#pragma once

#include "element.h"
#include <atomic>

class Terminal : public View
{
  public:
    Terminal();
    ~Terminal();

    void render(BaseElement e);
    void clear();
    void runInteractive(BaseElement e);
    void stop() { running = false; }

    static KeyEvent keyPress();

    void write(std::size_t column, std::size_t row, Style style, std::string_view data) override;
    void printStyle(const Style &style);

  private:
    std::atomic<bool> running;
};
