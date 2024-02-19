#pragma once

#include <fcntl.h>
#include <format>
#include <iostream>
#include <string.h>
#include <string>
#include <string_view>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

namespace wibens::tuilight::ansi
{
enum class ColorCode : unsigned {
    Black = 30,
    Red = 31,
    Green = 32,
    Yellow = 33,
    Blue = 34,
    Magenta = 35,
    Cyan = 36,
    White = 37,
    Reset = 39
};
enum class StyleCode : unsigned {
    Reset = 0,
    Bold = 1,
    Dim = 2,
    Italic = 3,
    Underline = 4,
    Blink = 5,
    Invert = 7,
    Hidden = 8,
};

inline void setForegroundColor(ColorCode color) { std::cout << std::format("\033[{}m", static_cast<unsigned>(color)); }
inline void setBackgroundColor(ColorCode color)
{
    std::cout << std::format("\033[{}m", static_cast<unsigned>(color) + 10);
}
inline void setStyle(StyleCode style) { std::cout << std::format("\033[{}m", static_cast<unsigned>(style)); }
inline void showCursor(bool show)
{
    if (show) {
        std::cout << "\033[?25h";
    } else {
        std::cout << "\033[?25l";
    }
}
inline void clear() { std::cout << "\033[2J"; }
inline void moveCursor(int x, int y) { std::cout << std::format("\033[{};{}H", y + 1, x + 1); }

struct TerminalSize {
    std::size_t rows;
    std::size_t cols;
};

struct TerminalRestorer {
    TerminalRestorer() { tcgetattr(STDIN_FILENO, &original); }
    TerminalRestorer(const TerminalRestorer &) = delete;
    TerminalRestorer(TerminalRestorer &&) = default;
    inline ~TerminalRestorer() { tcsetattr(STDIN_FILENO, TCSAFLUSH, &original); }
    auto getOriginal() { return original; }

  private:
    struct termios original;
};

inline TerminalSize getTerminalSize()
{
    struct winsize ws;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
    return {ws.ws_row, ws.ws_col};
}

[[nodiscard]] inline auto rawTerminal()
{
    TerminalRestorer restorer;
    struct termios raw = restorer.getOriginal();
    raw.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
    return restorer;
}

enum class KeyEvent {
    INTERRUPT = -3,
    TIMEOUT = -2,
    UNKNOWN = -1,
    BACKSPACE = 8,
    TAB = 9,
    RETURN = 10,
    ESCAPE = 27,
    SPACE = 32,
    INSERT = 256,
    DELETE,
    UP,
    DOWN,
    RIGHT,
    LEFT,
    END,
    HOME,
    BACKTAB,
    PAGE_UP,
    PAGE_DOWN,
    F1,
    F2,
    F3,
    F4,
    F5,
    F6,
    F7,
    F8,
    F9,
    F10,
    F11,
    F12,
};
inline KeyEvent CharEvent(char c) { return static_cast<KeyEvent>(c); }

} // namespace wibens::tuilight::ansi
