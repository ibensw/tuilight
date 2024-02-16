#include "terminal.h"
#include "ansi.h"
#include <iostream>
#include <poll.h>

Terminal::Terminal()
    : View([] { return ANSIControlCodes::getTerminalSize().cols; }(),
           [] { return ANSIControlCodes::getTerminalSize().rows; }()) // TODO
{
    ANSIControlCodes::enterRawMode();
    std::cout << ANSIControlCodes::HIDE_CURSOR;
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
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
        auto key = keyPress();
        // if (key == KeyEvent::ESCAPE) {
        //     running = false;
        // }
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

// Function to read a key press event
KeyEvent Terminal::keyPress()
{
    const int TIMEOUT_MS = 1000; // Timeout in milliseconds

    // Set up the pollfd structure for monitoring stdin
    struct pollfd fds[1];
    fds[0].fd = STDIN_FILENO; // File descriptor for stdin
    fds[0].events = POLLIN;   // Poll for input events

    int ret = poll(fds, 1, TIMEOUT_MS);
    if (ret <= 0) {
        return KeyEvent::TIMEOUT;
    }

    char c = getchar();
    if (c != 0x1b) {
        return CharEvent(c);
    }
    char seq[4];
    for (int i = 0; i < 4; ++i) {
        seq[i] = getchar();
    }
    if (seq[0] != 0x5b) {
        return KeyEvent::ESCAPE;
    }

    switch (seq[1]) {
        case 0x32:
            return KeyEvent::INSERT;
        case 0x33:
            return KeyEvent::DELETE;
        case 0x41:
            return KeyEvent::UP;
        case 0x42:
            return KeyEvent::DOWN;
        case 0x43:
            return KeyEvent::RIGHT;
        case 0x44:
            return KeyEvent::LEFT;
        case 0x46:
            return KeyEvent::END;
        case 0x48:
            return KeyEvent::HOME;
        case 0x5a:
            return KeyEvent::BACKTAB;
    };
    switch (seq[1]) {
        case 0x35:
            return KeyEvent::PAGE_UP;
        case 0x36:
            return KeyEvent::PAGE_DOWN;
    }
    if (seq[1] == 0x31) {
        return static_cast<KeyEvent>(static_cast<int>(KeyEvent::F1) + seq[2] - 0x31);
    }
    if (seq[1] == 0x32) {
        return static_cast<KeyEvent>(static_cast<int>(KeyEvent::F9) + seq[2] - 0x30);
    }
    return KeyEvent::UNKNOWN;
}
