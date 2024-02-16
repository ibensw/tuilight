#pragma once

#include <fcntl.h>
#include <iostream>
#include <string.h>
#include <string>
#include <string_view>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

namespace ANSIControlCodes
{

// Text formatting
constexpr std::string_view RESET = "\033[0m";
constexpr std::string_view BOLD = "\033[1m";
constexpr std::string_view DIM = "\033[2m";
constexpr std::string_view UNDERLINE = "\033[4m";
constexpr std::string_view BLINK = "\033[5m";
constexpr std::string_view INVERT = "\033[7m";
constexpr std::string_view HIDDEN = "\033[8m";

// Foreground colors
constexpr std::string_view FG_BLACK = "\033[30m";
constexpr std::string_view FG_RED = "\033[31m";
constexpr std::string_view FG_GREEN = "\033[32m";
constexpr std::string_view FG_YELLOW = "\033[33m";
constexpr std::string_view FG_BLUE = "\033[34m";
constexpr std::string_view FG_MAGENTA = "\033[35m";
constexpr std::string_view FG_CYAN = "\033[36m";
constexpr std::string_view FG_WHITE = "\033[37m";

// Background colors
constexpr std::string_view BG_BLACK = "\033[40m";
constexpr std::string_view BG_RED = "\033[41m";
constexpr std::string_view BG_GREEN = "\033[42m";
constexpr std::string_view BG_YELLOW = "\033[43m";
constexpr std::string_view BG_BLUE = "\033[44m";
constexpr std::string_view BG_MAGENTA = "\033[45m";
constexpr std::string_view BG_CYAN = "\033[46m";
constexpr std::string_view BG_WHITE = "\033[47m";

// Cursor
constexpr std::string_view HIDE_CURSOR = "\033[?25l";
constexpr std::string_view SHOW_CURSOR = "\033[?25h";

// Clear the screen
constexpr std::string_view clearScreen = "\033[2J\033[1;1H";

// Move cursor to specified position (0-based index)
inline std::string moveCursorTo(int col, int row)
{
    return "\033[" + std::to_string(row + 1) + ";" + std::to_string(col + 1) + "H";
}

// // Move cursor up by 'n' rows
// std::string moveCursorUp(int n) {
//     return "\033[" + std::to_string(n) + "A";
// }

// // Move cursor down by 'n' rows
// std::string moveCursorDown(int n) {
//     return "\033[" + std::to_string(n) + "B";
// }

// // Move cursor forward by 'n' columns
// std::string moveCursorForward(int n) {
//     return "\033[" + std::to_string(n) + "C";
// }

// // Move cursor backward by 'n' columns
// std::string moveCursorBackward(int n) {
//     return "\033[" + std::to_string(n) + "D";
// }

// // Save current cursor position
// constexpr std::string_view saveCursorPosition() {
//     return "\033[s";
// }

// // Restore cursor to saved position
// constexpr std::string_view restoreCursorPosition() {
//     return "\033[u";
// }

struct TerminalSize {
    std::size_t rows;
    std::size_t cols;
};

inline TerminalSize getTerminalSize()
{
    struct winsize ws;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
    return {ws.ws_row, ws.ws_col};
}

// Function to enter raw mode
static struct termios original_termios;

inline void enterRawMode()
{
    tcgetattr(STDIN_FILENO, &original_termios);
    struct termios raw = original_termios;
    raw.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

// Function to restore terminal settings to normal
inline void restoreTerminalSettings() { tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_termios); }

} // namespace ANSIControlCodes

enum class KeyEvent {
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
