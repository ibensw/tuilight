#include "terminal.h"
#include "ansi.h"
#include <iostream>

Terminal::Terminal()
    : View([] { return ANSIControlCodes::getTerminalSize().cols; }(),
           [] { return ANSIControlCodes::getTerminalSize().rows; }()) // TODO
{
    ANSIControlCodes::enterRawMode();
    std::cout << ANSIControlCodes::HIDE_CURSOR;
}

Terminal::~Terminal()
{
    ANSIControlCodes::restoreTerminalSettings();
    std::cout << ANSIControlCodes::RESET << ANSIControlCodes::SHOW_CURSOR;
}

void Terminal::render(BaseElement e)
{
    std::cout << ANSIControlCodes::clearScreen;
    auto size = ANSIControlCodes::getTerminalSize();
    // TerminalView view{size.cols, size.rows};
    e->render(*this);
    std::cout.flush();
}

void Terminal::clear()
{
    std::cout << ANSIControlCodes::clearScreen;
    std::cout.flush();
}

void Terminal::runInteractive(BaseElement e)
{
    running = true;
    e = NoEscape(e);
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

void Terminal::write(std::size_t column, std::size_t row, Style style, std::string_view data)
{
    std::cout << ANSIControlCodes::moveCursorTo(column, row);
    printStyle(style);
    std::cout << data;
};
void Terminal::printStyle(const Style &style)
{
    std::cout << ANSIControlCodes::RESET;
    if (style.bold) {
        std::cout << ANSIControlCodes::BOLD;
    }
    if (style.underline) {
        std::cout << ANSIControlCodes::UNDERLINE;
    }
    if (style.blink) {
        std::cout << ANSIControlCodes::BLINK;
    }
    if (style.dim) {
        std::cout << ANSIControlCodes::DIM;
    }
    if (style.invert) {
        std::cout << ANSIControlCodes::INVERT;
    }
    if (style.hidden) {
        std::cout << ANSIControlCodes::HIDDEN;
    }
    if (style.fgColor.has_value()) {
        std::cout << style.fgColor.value();
    }
    if (style.bgColor.has_value()) {
        std::cout << style.bgColor.value();
    }
}
