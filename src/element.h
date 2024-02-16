#pragma once

#include "ansi.h"
#include <algorithm>
#include <limits>
#include <memory>
#include <optional>
#include <stack>
#include <string_view>
#include <vector>

// class Percent
// {
//   public:
//     Percent(double value) : value(value) {}
//     operator double() const { return value; }
//     operator double &() { return value; }

//   private:
//     double value;
// };

struct Size {
    Size() = default;
    Size(std::size_t width, std::size_t height) : minWidth(width), maxWidth(width), minHeight(height), maxHeight(height)
    {
    }
    Size(std::size_t minWidth, std::size_t minHeight, std::size_t maxWidth, std::size_t maxHeight)
        : minWidth(minWidth), minHeight(minHeight), maxWidth(maxWidth), maxHeight(maxHeight)
    {
    }
    std::size_t minWidth{};
    std::size_t minHeight{};
    std::size_t maxWidth{};
    std::size_t maxHeight{};
};

struct Style {
    bool bold = false;
    bool underline = false;
    bool blink = false;
    bool dim = false;
    bool invert = false;
    bool hidden = false;
    std::optional<std::string> fgColor{};
    std::optional<std::string> bgColor{};
};

#include <iostream>
class View
{
  public:
    View(std::size_t width, std::size_t height, Style style = {}) : width(width), height(height), viewStyle(style) {}
    virtual ~View() = default;
    virtual void write(std::size_t column, std::size_t row, Style style, std::string_view data) = 0;

    const std::size_t width;
    const std::size_t height;

    Style viewStyle;
};

class TerminalView : public View
{
  public:
    TerminalView(std::size_t width, std::size_t height) : View(width, height) {}
    void write(std::size_t column, std::size_t row, Style style, std::string_view data) override
    {
        std::cout << ANSIControlCodes::moveCursorTo(column, row);
        printStyle(style);
        std::cout << data;
    };
    void printStyle(const Style &style)
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
};

class SubView : public View
{
  public:
    SubView(View &parent, std::size_t x, std::size_t y, std::size_t width, std::size_t height)
        : View(width, height, parent.viewStyle), parent(parent), x(x), y(y)
    {
    }

    void write(std::size_t column, std::size_t row, Style style, std::string_view data) override
    {
        if (column < width && row < height) {
            if (column + data.size() > width) {
                data = data.substr(0, width - column);
            }
            parent.write(column + x, row + y, style, data);
        }
    }

    const std::size_t x;
    const std::size_t y;

  private:
    View &parent;
};

class BaseElementImpl
{
  public:
    virtual void render(View &view) = 0;
    virtual Size getSize() const = 0;
    virtual bool focusable() const { return false; }
    virtual void setFocus(bool focus) { focused = focus; }
    bool isFocused() const { return focused; }
    virtual bool handleEvent(int event) { return false; }

  private:
    bool focused = false;
};
using BaseElement = std::shared_ptr<BaseElementImpl>;

struct DecoratorImpl : BaseElementImpl {
    BaseElement inner;
    DecoratorImpl(BaseElement inner) : inner(std::move(inner)) {}
    Size getSize() const override { return inner->getSize(); };
    bool focusable() const override { return inner->focusable(); }
    bool handleEvent(int event) override { return inner->handleEvent(event); }
    void setFocus(bool focused) override
    {
        BaseElementImpl::setFocus(focused);
        return inner->setFocus(focused);
    };
};
using BaseDecorator = std::shared_ptr<DecoratorImpl>;

template <class D> class Element : public std::shared_ptr<D>
{
  public:
    using Type = D;
    Element(std::shared_ptr<D> ptr) : std::shared_ptr<D>(std::move(ptr)) {}
    template <class... Types> Element(Types... args) : std::shared_ptr<D>(std::make_shared<D>(args...)) {}
    operator BaseElement() const { return std::static_pointer_cast<BaseElementImpl>(*this); }
    template <class T> auto operator|(T decorator) { return decorator(*this); }
};

auto Center(BaseElement inner)
{
    class Impl : public DecoratorImpl
    {
      public:
        using DecoratorImpl::DecoratorImpl;
        void render(View &view) override
        {
            SubView sv(view, view.width / 2 - inner->getSize().minWidth / 2, 0, inner->getSize().minWidth,
                       inner->getSize().minHeight);
            inner->render(sv);
        }
    };
    return Element<Impl>(std::make_shared<Impl>(inner));
}

auto Color(std::string_view color)
{
    return [color](BaseElement inner) {
        class Impl : public DecoratorImpl
        {
          public:
            Impl(BaseElement inner, std::string_view color) : DecoratorImpl(inner), color(color) {}
            void render(View &view) override
            {
                view.viewStyle.fgColor = color;
                inner->render(view);
            }

          private:
            std::string_view color;
        };
        return Element<Impl>(std::make_shared<Impl>(inner, color));
    };
}

auto Text(const std::string &other, bool fill = false)
{
    class Impl : public BaseElementImpl
    {
      public:
        Impl(std::string text, bool fill = false) : text(text), fill(fill) {}
        void render(View &view) override
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
        Size getSize() const override { return {text.size(), 1}; }

        std::string text;
        bool fill;
    };

    return Element<Impl>(other, fill);
}

template <typename T> auto Button(const std::string &label, T action)
{
    using TextImpl = decltype(Text(""))::Type;
    class Impl : public TextImpl
    {
      public:
        Impl(std::string text, T action) : TextImpl("[ " + text + " ]"), action(action) {}
        bool focusable() const override { return true; }
        void render(View &view) override
        {
            if (isFocused()) {
                view.viewStyle.invert = !view.viewStyle.invert;
                TextImpl::render(view);
            } else {
                TextImpl::render(view);
            }
        }
        bool handleEvent(int event) override
        {
            switch (event) {
                case '\n':
                case ' ':
                    action();
                    return true;
                default:
                    return false;
            }
        }

        T action;
    };
    return Element<Impl>(label, action);
}

auto VContainer(const std::vector<BaseElement> &elements)
{
    class Impl : public BaseElementImpl
    {
      public:
        Impl(const std::vector<BaseElement> &elements) : elements(elements)
        {
            std::for_each(elements.cbegin(), elements.cend(), [this](const BaseElement &e) {
                if (e->focusable()) {
                    this->focusableChildren.push_back(e);
                }
            });
        }

        void render(View &view) override
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

        Size getSize() const override
        {
            Size size{};
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

        virtual bool focusable() const { return !focusableChildren.empty(); }
        virtual void setFocus(bool focus)
        {
            focusableChildren[focusedElement]->setFocus(focus);
            BaseElementImpl::setFocus(focus);
        }
        virtual bool handleEvent(int event)
        {
            if (focusableChildren[focusedElement]->handleEvent(event)) {
                return true;
            }
            switch (event) {
                case '8':
                    focusableChildren[focusedElement]->setFocus(false);
                    if (focusedElement > 0) {
                        --focusedElement;
                        focusableChildren[focusedElement]->setFocus(true);
                        return true;
                    }
                    break;
                case '2':
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
        BaseElement focusedChild() const { return focusableChildren.at(focusedElement); }

      private:
        std::vector<BaseElement> elements;
        std::vector<BaseElement> focusableChildren;
        std::size_t focusedElement{};
    };
    return Element<Impl>(std::make_shared<Impl>(elements));
}
template <class... Elements> auto VContainer(Elements... elements)
{
    return VContainer(std::vector<BaseElement>{elements...});
}

auto Bottom(BaseElement inner)
{
    class Impl : public DecoratorImpl
    {
      public:
        using DecoratorImpl::DecoratorImpl;
        void render(View &view) override
        {
            SubView subview{view, 0, view.height - inner->getSize().minHeight, inner->getSize().minWidth,
                            inner->getSize().minHeight};
            inner->render(subview);
        }
    };
    return Element<Impl>(std::make_shared<Impl>(inner));
}

auto Stretch(std::size_t maxWidth = std::numeric_limits<std::size_t>::max(),
             std::size_t maxHeight = std::numeric_limits<std::size_t>::max())
{
    return [=](BaseElement inner) {
        class Impl : public DecoratorImpl
        {
          public:
            Impl(BaseElement inner, std::size_t maxWidth, std::size_t maxHeight)
                : DecoratorImpl(inner), maxWidth(maxWidth), maxHeight(maxHeight)
            {
            }
            void render(View &view) override { inner->render(view); }
            Size getSize() const override
            {
                auto size = inner->getSize();
                size.maxWidth = std::max<std::size_t>(size.maxWidth, maxWidth);
                size.maxHeight = std::max<std::size_t>(size.maxHeight, maxHeight);
                return size;
            };

          private:
            std::size_t maxWidth;
            std::size_t maxHeight;
        };
        return Element<Impl>(std::make_shared<Impl>(inner, maxWidth, maxHeight));
    };
}

auto xStretch(std::size_t maxWidth = std::numeric_limits<std::size_t>::max())
{
    return Stretch(maxWidth, std::numeric_limits<std::size_t>::max());
}
auto yStretch(std::size_t maxHeight = std::numeric_limits<std::size_t>::max())
{
    return Stretch(std::numeric_limits<std::size_t>::max(), maxHeight);
}

auto Limit(std::size_t maxWidth = std::numeric_limits<std::size_t>::max(),
           std::size_t maxHeight = std::numeric_limits<std::size_t>::max())
{
    return [=](BaseElement inner) {
        class Impl : public DecoratorImpl
        {
          public:
            Impl(BaseElement inner, std::size_t maxWidth, std::size_t maxHeight)
                : DecoratorImpl(inner), maxWidth(maxWidth), maxHeight(maxHeight)
            {
            }
            void render(View &view) override
            {
                if (view.width > maxWidth || view.height > maxHeight) {
                    SubView sv(view, 0, 0, std::min(maxWidth, view.width), std::min(maxHeight, view.height));
                    inner->render(sv);
                } else {
                    inner->render(view);
                }
            }

            Size getSize() const override
            {
                auto size = inner->getSize();
                size.minWidth = std::min(size.minWidth, maxWidth);
                size.minHeight = std::min(size.minHeight, maxHeight);
                size.maxWidth = std::min(size.maxWidth, maxWidth);
                size.maxHeight = std::min(size.maxHeight, maxHeight);
                return size;
            };

          private:
            std::size_t maxWidth;
            std::size_t maxHeight;
        };
        return Element<Impl>(std::make_shared<Impl>(inner, maxWidth, maxHeight));
    };
}

auto makeStyle(void (*modifier)(Style &))
{
    return [modifier](BaseElement inner) {
        class Impl : public DecoratorImpl
        {
          public:
            Impl(BaseElement inner, void (*modifier)(Style &)) : DecoratorImpl(inner), modifier(modifier) {}
            void render(View &view) override
            {
                modifier(view.viewStyle);
                inner->render(view);
            }
            void (*modifier)(Style &);
        };
        return Element<Impl>(std::make_shared<Impl>(inner, modifier));
    };
}

const auto Bold = makeStyle([](Style &s) { s.bold = true; });
const auto Dim = makeStyle([](Style &s) { s.dim = true; });
const auto Underline = makeStyle([](Style &s) { s.underline = true; });
const auto Blink = makeStyle([](Style &s) { s.blink = true; });
const auto Invert = makeStyle([](Style &s) { s.invert = !s.invert; });
const auto Hidden = makeStyle([](Style &s) { s.hidden = true; });

auto ScrollBox(std::size_t width = std::numeric_limits<std::size_t>::max(),
               std::size_t height = std::numeric_limits<std::size_t>::max(), std::size_t *xOffset = nullptr,
               std::size_t *yOffset = nullptr, bool xScrollbar = false, bool yScrollbar = false)
{
    return [=](BaseElement inner) {
        class Impl : public DecoratorImpl
        {
          public:
            Impl(BaseElement inner, std::size_t width, std::size_t height, std::size_t *xOffset, std::size_t *yOffset,
                 bool xScrollbar, bool yScrollbar)
                : DecoratorImpl(inner), width(width), height(height), xOffset(xOffset), yOffset(yOffset),
                  xScrollbar(xScrollbar), yScrollbar(yScrollbar)
            {
            }
            void render(View &view) override
            {
                class ScrollView : public SubView
                {
                  public:
                    ScrollView(View &view, std::size_t width, std::size_t height, std::size_t xScroll,
                               std::size_t yScroll)
                        : SubView(view, 0, 0, width, height), xScroll(xScroll), yScroll(yScroll)
                    {
                    }
                    std::size_t xScroll;
                    std::size_t yScroll;

                    void write(std::size_t column, std::size_t row, Style style, std::string_view data) override
                    {
                        SubView::write(column - xScroll, row - yScroll, style, data);
                    }
                };

                auto viewWidth = view.width;
                auto innerHeight = inner->getSize().minHeight;
                if (yOffset && yScrollbar && innerHeight > view.height) {
                    auto maxScroll = innerHeight - view.height;
                    if (*yOffset > maxScroll) {
                        *yOffset = maxScroll;
                    }
                    auto scrollBarSize = std::max<std::size_t>(1, view.height * view.height / innerHeight);
                    auto scrollOffset = ((view.height - scrollBarSize) * *yOffset + maxScroll / 2) / (maxScroll);
                    while (scrollBarSize > 0) {
                        view.write(view.width - 1, scrollOffset, view.viewStyle, "#");
                        --scrollBarSize;
                        ++scrollOffset;
                    }
                    viewWidth--;
                }
                ScrollView subview(view, viewWidth, view.height, xOffset ? *xOffset : 0, yOffset ? *yOffset : 0);
                inner->render(subview);
            }
            Size getSize() const override
            {
                auto size = inner->getSize();
                if (yScrollbar) {
                    size.minWidth++;
                    size.maxWidth = std::max(size.maxWidth, size.maxWidth + 1);
                }
                size.minWidth = std::min<std::size_t>(size.minWidth, width);
                size.minHeight = std::min<std::size_t>(size.minHeight, height);
                size.maxWidth = std::min<std::size_t>(size.maxWidth, width);
                size.maxHeight = std::min<std::size_t>(size.maxHeight, height);
                return size;
            };

          private:
            std::size_t width;
            std::size_t height;
            std::size_t *xOffset = nullptr;
            std::size_t *yOffset = nullptr;
            bool xScrollbar = false;
            bool yScrollbar = false;
        };
        return Element<Impl>(std::make_shared<Impl>(inner, width, height, xOffset, yOffset, xScrollbar, yScrollbar));
    };
}

auto vScroll = [](std::size_t height, std::size_t *yOffset = nullptr, bool scrollbar = false) {
    return ScrollBox(std::numeric_limits<std::size_t>::max(), height, nullptr, yOffset, false, scrollbar);
};

auto Frame(BaseElement inner)
{
    class Impl : public DecoratorImpl
    {
      public:
        using DecoratorImpl::DecoratorImpl;
        void render(View &view) override
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
        Size getSize() const override
        {
            auto size = inner->getSize();
            size.minWidth = size.minWidth + 2;
            size.minHeight = size.minHeight + 2;
            size.maxWidth = std::max(size.maxWidth, size.maxWidth + 2);
            size.maxHeight = std::max(size.maxHeight, size.maxHeight + 2);
            return size;
        };
    };
    return Element<Impl>(std::make_shared<Impl>(inner));
}

auto Selectable(BaseElement inner)
{
    class Impl : public DecoratorImpl
    {
      public:
        Impl(BaseElement inner) : DecoratorImpl(inner) {}

        bool focusable() const override { return true; }

        void render(View &view) override
        {
            if (isFocused()) {
                Invert(inner)->render(view);
            } else {
                return inner->render(view);
            }
        }

        Size getSize() const override { return inner->getSize(); };
    };
    return Element<Impl>(std::make_shared<Impl>(inner));
}

auto VMenu(const std::vector<BaseElement> &elements)
{
    class Impl : public BaseElementImpl
    {
      public:
        Impl(const std::vector<BaseElement> &elements) : elements(elements) {}

        void render(View &view) override
        {
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

        Size getSize() const override
        {
            Size size{};
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

        virtual bool focusable() const { return !elements.empty(); }
        virtual void setFocus(bool focus)
        {
            elements[focusedIndex]->setFocus(focus);
            BaseElementImpl::setFocus(focus);
        }
        bool next()
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
        bool prev()
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
        virtual bool handleEvent(int event)
        {
            if (elements[focusedIndex]->handleEvent(event)) {
                return true;
            }
            switch (event) {
                case '8':
                    elements[focusedIndex]->setFocus(false);
                    if (focusedIndex > 0) {
                        --focusedIndex;
                        elements[focusedIndex]->setFocus(true);
                        return true;
                    }
                    break;
                case '2':
                    elements[focusedIndex]->setFocus(false);
                    if (focusedIndex < elements.size() - 1) {
                        ++focusedIndex;
                        elements[focusedIndex]->setFocus(true);
                        return true;
                    }
                    break;
            }
            return false;
        }
        BaseElement focusedChild() const { return elements.at(focusedIndex); }

      private:
        std::vector<BaseElement> elements;
        std::size_t focusedIndex{};
        std::size_t scrolledValue{};
        mutable std::vector<long> offsets;
    };
    return Element<Impl>(std::make_shared<Impl>(elements));
}