#include "view.h"

SubView::SubView(View &parent, std::size_t x, std::size_t y, std::size_t width, std::size_t height)
    : View(width, height, parent.viewStyle), parent(parent), x(x), y(y)
{
}

void SubView::write(std::size_t column, std::size_t row, Style style, std::string_view data)
{
    if (column < width && row < height) {
        if (column + data.size() > width) {
            data = data.substr(0, width - column);
        }
        parent.write(column + x, row + y, style, data);
    }
}
