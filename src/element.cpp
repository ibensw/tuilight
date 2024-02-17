#include "element.h"

namespace detail
{
void Center::render(View &view)
{
    SubView sv(view, view.width / 2 - inner->getSize().minWidth / 2, 0, inner->getSize().minWidth,
               inner->getSize().minHeight);
    inner->render(sv);
}

void Color::render(View &view)
{
    view.viewStyle.fgColor = color;
    inner->render(view);
}

void Text::render(View &view)
{
    view.write(0, 0, view.viewStyle, text);
    if (fill) {
        if (text.size() < view.width) {
            view.write(text.size(), 0, view.viewStyle, std::string(view.width - text.size(), ' '));
        }
        std::string empty(view.width, ' ');
        for (std::size_t i = 1; i < view.height; ++i) {
            view.write(0, i, view.viewStyle, empty);
        }
    }
}

void Button::render(View &view)
{
    if (isFocused()) {
        view.viewStyle.invert = !view.viewStyle.invert;
        Text::render(view);
    } else {
        Text::render(view);
    }
}
bool Button::handleEvent(KeyEvent event)
{
    switch (event) {
        case KeyEvent::RETURN:
        case KeyEvent::SPACE:
            action();
            return true;
        default:
            return false;
    }
}

VContainer::VContainer(const std::vector<BaseElement> &elements) : elements(elements)
{
    std::for_each(elements.cbegin(), elements.cend(), [this](const BaseElement &e) {
        if (e->focusable()) {
            this->focusableChildren.push_back(e);
        }
    });
}

void VContainer::render(View &view)
{
    std::size_t offset{};
    std::size_t slack{};
    auto size = getSize();
    if (view.height > size.minHeight) {
        slack = view.height - size.minHeight;
    }
    for (auto &element : elements) {
        auto elemSize = element->getSize();
        std::size_t height = elemSize.minHeight;
        if (elemSize.maxHeight > height && slack > 0) {
            auto extraHeight = std::min<std::size_t>(elemSize.maxHeight - elemSize.minHeight, slack);
            height += extraHeight;
            slack -= extraHeight;
        }
        // height = std::min<std::size_t>(height, view.height - offset);
        SubView subview{view, 0, offset, view.width, height};
        element->render(subview);
        offset += height;
    }
}

ElementSize VContainer::getSize() const
{
    ElementSize size{};
    for (auto &element : elements) {
        auto elemSize = element->getSize();
        size.minWidth = std::max(size.minWidth, elemSize.minWidth);
        size.minHeight += elemSize.minHeight;
        size.maxWidth = std::max(size.maxWidth, elemSize.maxWidth);
        size.maxHeight += elemSize.maxHeight;
        if (size.maxHeight < elemSize.maxHeight) {
            // overflow
            size.maxHeight = std::numeric_limits<std::size_t>::max();
        }
    }
    return size;
}

bool VContainer::handleEvent(KeyEvent event)
{
    if (focusableChildren[focusedElement]->handleEvent(event)) {
        return true;
    }
    switch (event) {
        case KeyEvent::UP:
        case KeyEvent::BACKTAB:
            focusableChildren[focusedElement]->setFocus(false);
            if (focusedElement > 0) {
                --focusedElement;
                focusableChildren[focusedElement]->setFocus(true);
                return true;
            }
            break;
        case KeyEvent::DOWN:
        case KeyEvent::TAB:
            focusableChildren[focusedElement]->setFocus(false);
            if (focusedElement < focusableChildren.size() - 1) {
                ++focusedElement;
                focusableChildren[focusedElement]->setFocus(true);
                return true;
            }
            break;
    }
    return false;
}

void Bottom::render(View &view)
{
    SubView subview{view, 0, view.height - inner->getSize().minHeight, inner->getSize().minWidth,
                    inner->getSize().minHeight};
    inner->render(subview);
}

ElementSize Stretch::getSize() const
{
    auto size = inner->getSize();
    size.maxWidth = std::max<std::size_t>(size.maxWidth, maxWidth);
    size.maxHeight = std::max<std::size_t>(size.maxHeight, maxHeight);
    return size;
};

void Limit::render(View &view)
{
    if (view.width > maxWidth || view.height > maxHeight) {
        SubView sv(view, 0, 0, std::min(maxWidth, view.width), std::min(maxHeight, view.height));
        inner->render(sv);
    } else {
        inner->render(view);
    }
}

ElementSize Limit::getSize() const
{
    auto size = inner->getSize();
    size.minWidth = std::min(size.minWidth, maxWidth);
    size.minHeight = std::min(size.minHeight, maxHeight);
    size.maxWidth = std::min(size.maxWidth, maxWidth);
    size.maxHeight = std::min(size.maxHeight, maxHeight);
    return size;
};

void Styler::render(View &view)
{
    modifier(view.viewStyle);
    inner->render(view);
}

void Frame::render(View &view)
{
    std::string horizontal = "#" + std::string(view.width - 2, '-') + "#";
    view.write(0, 0, view.viewStyle, horizontal);
    view.write(0, view.height - 1, view.viewStyle, horizontal);
    for (std::size_t i = 1; i < view.height - 1; i++) {
        view.write(0, i, view.viewStyle, "|");
        view.write(view.width - 1, i, view.viewStyle, "|");
    }
    SubView sv(view, 1, 1, view.width - 2, view.height - 2);
    inner->render(sv);
}
ElementSize Frame::getSize() const
{
    auto size = inner->getSize();
    size.minWidth = size.minWidth + 2;
    size.minHeight = size.minHeight + 2;
    size.maxWidth = std::max(size.maxWidth, size.maxWidth + 2);
    size.maxHeight = std::max(size.maxHeight, size.maxHeight + 2);
    return size;
};

void Selectable::render(View &view)
{
    if (isFocused()) {
        Invert(inner)->render(view);
    } else {
        return inner->render(view);
    }
}

void VMenu::render(View &view)
{
    pageSize = view.height;
    auto size = getSize();
    std::size_t slack{};
    if (size.minHeight > view.height) {
        scrolledValue = std::min(scrolledValue, size.minHeight - view.height);
        auto minScroll = offsets[focusedIndex + 1] - static_cast<long>(view.height);
        auto maxScroll = offsets[focusedIndex];
        scrolledValue = std::clamp<long>(scrolledValue, minScroll, maxScroll);
    } else {
        scrolledValue = 0;
        slack = view.height - size.minHeight;
    }

    auto offset = -scrolledValue;
    for (auto &element : elements) {
        auto elemSize = element->getSize();
        std::size_t height = elemSize.minHeight;
        if (elemSize.maxHeight > height && slack > 0) {
            auto extraHeight = std::min<std::size_t>(elemSize.maxHeight - elemSize.minHeight, slack);
            height += extraHeight;
            slack -= extraHeight;
        }
        // height = std::min<std::size_t>(height, view.height - offset);
        SubView subview{view, 0, offset, view.width, height};
        element->render(subview);
        offset += height;
    }
}

ElementSize VMenu::getSize() const
{
    ElementSize size{};
    offsets.clear();
    for (auto &element : elements) {
        offsets.push_back(size.minHeight);
        auto elemSize = element->getSize();
        size.minWidth = std::max(size.minWidth, elemSize.minWidth);
        size.minHeight += elemSize.minHeight;
        size.maxWidth = std::max(size.maxWidth, elemSize.maxWidth);
        size.maxHeight += elemSize.maxHeight;
        if (size.maxHeight < elemSize.maxHeight) {
            // overflow
            size.maxHeight = std::numeric_limits<std::size_t>::max();
        }
    }
    offsets.push_back(size.minHeight);
    return size;
}

bool VMenu::next()
{
    std::size_t newFocus = focusedIndex;
    elements[newFocus]->setFocus(false);
    while (newFocus < elements.size() - 1) {
        newFocus++;
        if (elements[newFocus]->focusable()) {
            focusedIndex = newFocus;
            elements[newFocus]->setFocus(true);
            return true;
        }
    }
    return false;
}
bool VMenu::prev()
{
    std::size_t newFocus = focusedIndex;
    elements[newFocus]->setFocus(false);
    while (newFocus > 0) {
        newFocus--;
        if (elements[newFocus]->focusable()) {
            focusedIndex = newFocus;
            elements[newFocus]->setFocus(true);
            return true;
        }
    }
    return false;
}
bool VMenu::handleEvent(KeyEvent event)
{
    if (elements[focusedIndex]->handleEvent(event)) {
        return true;
    }
    switch (event) {
        case KeyEvent::TAB:
        case KeyEvent::BACKTAB:
            elements[focusedIndex]->setFocus(false);
            return false;
        case KeyEvent::UP:
            return prev();
        case KeyEvent::DOWN:
            return next();
        case KeyEvent::PAGE_UP:
            if (focusedIndex > 0) {
                elements[focusedIndex]->setFocus(false);
                if (focusedIndex >= pageSize) {
                    focusedIndex -= pageSize;
                } else {
                    focusedIndex = 0;
                }
                elements[focusedIndex]->setFocus(true);
            }
            return true;
        case KeyEvent::PAGE_DOWN:
            if (focusedIndex < elements.size() - 1) {
                elements[focusedIndex]->setFocus(false);
                focusedIndex = std::min(elements.size() - 1, focusedIndex + pageSize);
                elements[focusedIndex]->setFocus(true);
            }
            return true;
            break;
    }
    return false;
}

bool NoEscape::handleEvent(KeyEvent event)
{
    if (!inner->handleEvent(event)) {
        switch (event) {
            case KeyEvent::UP:
            case KeyEvent::BACKTAB:
                inner->focusFirst();
                break;
            case KeyEvent::DOWN:
            case KeyEvent::TAB:
                inner->focusLast();
                break;
        }
    }
    return true;
}
}; // namespace detail