#pragma once
#include <optional>
#include <string>
#include <string_view>

namespace wibens::tuilight
{

enum class Color {
    Black,
    Red,
    Green,
    Yellow,
    Blue,
    Magenta,
    Cyan,
    White,
};

struct Style {
    bool bold = false;
    bool underline = false;
    bool blink = false;
    bool dim = false;
    bool invert = false;
    bool hidden = false;
    std::optional<Color> fgColor{};
    std::optional<Color> bgColor{};
};

class View
{
  public:
    View() = default;
    View(std::size_t width, std::size_t height, Style style = {}) : width(width), height(height), viewStyle(style) {}
    virtual ~View() = default;
    virtual void write(std::size_t column, std::size_t row, Style style, std::string_view data) = 0;

    std::size_t width{};
    std::size_t height{};

    Style viewStyle;
};

class SubView : public View
{
  public:
    SubView(View &parent, std::size_t x, std::size_t y, std::size_t width, std::size_t height);

    void write(std::size_t column, std::size_t row, Style style, std::string_view data) override;

    const std::size_t x;
    const std::size_t y;

  private:
    View &parent;
};

} // namespace wibens::tuilight
