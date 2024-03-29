#include "tuilight/element.h"

namespace wibens::tuilight::detail
{

void Center::render(View &view)
{
    SubView sv(view, view.width / 2 - inner->getSize().minWidth / 2, 0, inner->getSize().minWidth,
               inner->getSize().minHeight);
    inner->render(sv);
}

void ForegroundColor::render(View &view)
{
    view.viewStyle.fgColor = color;
    inner->render(view);
}

void BackgroundColor::render(View &view)
{
    view.viewStyle.bgColor = color;
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
bool Button::handleEvent(ansi::KeyEvent event)
{
    switch (event) {
        case ansi::KeyEvent::RETURN:
        case ansi::KeyEvent::SPACE:
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

void VContainer::focusChild(std::size_t index)
{
    focusableChildren[focusedElement]->setFocus(false);
    focusedElement = index;
    focusableChildren[focusedElement]->setFocus(true);
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

bool VContainer::handleEvent(ansi::KeyEvent event)
{
    if (focusableChildren[focusedElement]->handleEvent(event)) {
        return true;
    }
    switch (event) {
        case ansi::KeyEvent::UP:
        case ansi::KeyEvent::BACKTAB:
            focusableChildren[focusedElement]->setFocus(false);
            if (focusedElement > 0) {
                focusChild(focusedElement - 1);
                return true;
            }
            break;
        case ansi::KeyEvent::DOWN:
        case ansi::KeyEvent::TAB:
            focusableChildren[focusedElement]->setFocus(false);
            if (focusedElement < focusableChildren.size() - 1) {
                focusChild(focusedElement + 1);
                return true;
            }
            break;
    }
    return false;
}

void HContainer::render(View &view)
{
    std::size_t offset{};
    std::size_t slack{};
    auto size = getSize();
    if (view.width > size.minWidth) {
        slack = view.width - size.minWidth;
    }
    for (auto &element : elements) {
        auto elemSize = element->getSize();
        std::size_t width = elemSize.minWidth;
        if (elemSize.maxWidth > width && slack > 0) {
            auto extraWidth = std::min<std::size_t>(elemSize.maxWidth - elemSize.minWidth, slack);
            width += extraWidth;
            slack -= extraWidth;
        }
        SubView subview{view, offset, 0, width, view.height};
        element->render(subview);
        offset += width;
    }
}

ElementSize HContainer::getSize() const
{
    ElementSize size{};
    for (auto &element : elements) {
        auto elemSize = element->getSize();
        size.minHeight = std::max(size.minHeight, elemSize.minHeight);
        size.minWidth += elemSize.minWidth;
        size.maxHeight = std::max(size.maxHeight, elemSize.maxHeight);
        size.maxWidth += elemSize.maxWidth;
        if (size.maxWidth < elemSize.maxWidth) {
            // overflow
            size.maxWidth = std::numeric_limits<std::size_t>::max();
        }
    }
    return size;
}

bool HContainer::handleEvent(ansi::KeyEvent event)
{
    if (focusableChildren[focusedElement]->handleEvent(event)) {
        return true;
    }
    switch (event) {
        case ansi::KeyEvent::LEFT:
        case ansi::KeyEvent::BACKTAB:
            focusableChildren[focusedElement]->setFocus(false);
            if (focusedElement > 0) {
                --focusedElement;
                focusableChildren[focusedElement]->setFocus(true);
                return true;
            }
            break;
        case ansi::KeyEvent::RIGHT:
        case ansi::KeyEvent::TAB:
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

ElementSize Shrink::getSize() const
{
    auto size = inner->getSize();
    size.minWidth = std::min<std::size_t>(size.maxWidth, minWidth);
    size.minHeight = std::min<std::size_t>(size.maxHeight, minHeight);
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
bool VMenu::handleEvent(ansi::KeyEvent event)
{
    if (elements[focusedIndex]->handleEvent(event)) {
        return true;
    }
    switch (event) {
        case ansi::KeyEvent::TAB:
        case ansi::KeyEvent::BACKTAB:
            elements[focusedIndex]->setFocus(false);
            return false;
        case ansi::KeyEvent::UP:
            return prev();
        case ansi::KeyEvent::DOWN:
            return next();
        case ansi::KeyEvent::PAGE_UP:
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
        case ansi::KeyEvent::PAGE_DOWN:
            if (focusedIndex < elements.size() - 1) {
                elements[focusedIndex]->setFocus(false);
                focusedIndex = std::min(elements.size() - 1, focusedIndex + pageSize);
                elements[focusedIndex]->setFocus(true);
            }
            return true;
            break;
        case ansi::KeyEvent::HOME:
            if (focusedIndex > 0) {
                elements[focusedIndex]->setFocus(false);
                focusedIndex = 0;
                elements[focusedIndex]->setFocus(true);
            }
            break;
        case ansi::KeyEvent::END:
            if (focusedIndex < elements.size() - 1) {
                elements[focusedIndex]->setFocus(false);
                focusedIndex = elements.size() - 1;
                elements[focusedIndex]->setFocus(true);
            }
            break;
    }
    return false;
}

bool NoEscape::handleEvent(ansi::KeyEvent event)
{
    if (!inner->handleEvent(event)) {
        switch (event) {
            case ansi::KeyEvent::UP:
            case ansi::KeyEvent::BACKTAB:
            case ansi::KeyEvent::LEFT:
                inner->focusFirst();
                break;
            case ansi::KeyEvent::DOWN:
            case ansi::KeyEvent::TAB:
            case ansi::KeyEvent::RIGHT:
                inner->focusLast();
                break;
        }
    }
    return true;
}

} // namespace wibens::tuilight::detail
