#include "tuilight/terminal.h"
#include "tuilight/ansi.h"
#include <csignal>
#include <iostream>
#include <poll.h>
#include <stdexcept>

namespace wibens::tuilight
{
using namespace ansi;

static Terminal *handlingTerminal = nullptr;

Terminal::Terminal() : restore(rawTerminal())
{
    showCursor(false);
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);

    handlingTerminal = this;
    struct sigaction sa;
    sa.sa_handler = [](int sig) {
        if (handlingTerminal != nullptr) {
            handlingTerminal->post([](Terminal &, BaseElement) {});
        }
    };
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGWINCH, &sa, nullptr) == -1) {
        throw std::system_error(errno, std::generic_category(), "sigaction failed");
    }

    if (pipe(pipeFd) == -1) {
        throw std::system_error(errno, std::generic_category(), "pipe failed");
    }
}

Terminal::~Terminal()
{
    showCursor(true);
    handlingTerminal = nullptr;
}

void Terminal::render(BaseElement e)
{
    clear();
    moveCursor(0, 0);
    auto size = getTerminalSize();
    width = size.cols;
    height = size.rows;
    e->render(*this);
    std::cout.flush();
}

void Terminal::clear() { ansi::clear(); }

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
        if (key > KeyEvent::UNKNOWN) {
            e->handleEvent(key);
        }
        while (!callbacks.empty()) {
            callbacks.back()(*this, e);
            callbacks.pop_back();
        }
    }
}

void Terminal::write(std::size_t column, std::size_t row, Style style, std::string_view data)
{
    moveCursor(column, row);
    printStyle(style);
    std::cout << data;
};
void Terminal::printStyle(const Style &style)
{
    setStyle(StyleCode::Reset);
    if (style.bold) {
        setStyle(StyleCode::Bold);
    }
    if (style.underline) {
        setStyle(StyleCode::Underline);
    }
    if (style.blink) {
        setStyle(StyleCode::Blink);
    }
    if (style.dim) {
        setStyle(StyleCode::Dim);
    }
    if (style.invert) {
        setStyle(StyleCode::Invert);
    }
    if (style.hidden) {
        setStyle(StyleCode::Hidden);
    }
    if (style.fgColor.has_value()) {
        auto code = static_cast<unsigned>(style.fgColor.value()) + static_cast<unsigned>(ColorCode::Black);
        if (style.fgColor.value() >= Color::Gray) {
            code = static_cast<unsigned>(style.fgColor.value()) - static_cast<unsigned>(Color::Gray) +
                   static_cast<unsigned>(ColorCode::Gray);
        }
        auto color = static_cast<ColorCode>(code);
        setForegroundColor(color);
    }
    if (style.bgColor.has_value()) {
        auto code = static_cast<unsigned>(style.bgColor.value()) + static_cast<unsigned>(ColorCode::Black);
        if (style.bgColor.value() >= Color::Gray) {
            code = static_cast<unsigned>(style.bgColor.value()) - static_cast<unsigned>(Color::Gray) +
                   static_cast<unsigned>(ColorCode::Gray);
        }
        auto color = static_cast<ColorCode>(code);
        setBackgroundColor(color);
    }
}

void Terminal::post(std::function<void(Terminal &, BaseElement)> fun)
{
    callbacks.push_front(fun);
    char c = 'E';
    if (::write(pipeFd[1], &c, 1) == -1) {
        throw std::system_error(errno, std::generic_category(), "write failed");
    }
}

void Terminal::postKeyPress(KeyEvent event)
{
    post([event](Terminal &, BaseElement e) { e->handleEvent(event); });
}

// Function to read a key press event
KeyEvent Terminal::keyPress()
{
    // Set up the pollfd structure for monitoring stdin
    std::array<struct pollfd, 2> pollFds;
    pollFds[0].fd = STDIN_FILENO; // File descriptor for stdin
    pollFds[0].events = POLLIN;   // Poll for input events
    pollFds[1].fd = pipeFd[0];    // File descriptor for pipe
    pollFds[1].events = POLLIN;   // Poll for input events

    int ret = poll(pollFds.data(), pollFds.size(), -1);
    if (ret == 0) {
        return KeyEvent::TIMEOUT;
    }
    if (pollFds[1].revents) {
        char c;
        if (::read(pollFds[1].fd, &c, 1) == -1) {
            throw std::system_error(errno, std::generic_category(), "read failed");
        }
        return KeyEvent::INTERRUPT;
    }

    auto c = getchar();
    if (c != 0x1b) {
        return CharEvent(c);
    }
    std::array<char, 4> seq;
    for (auto &s : seq) {
        s = getchar();
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

} // namespace wibens::tuilight
