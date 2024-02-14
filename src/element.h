#pragma once

#include "ansi.h"
#include <limits>
#include <memory>
#include <string_view>
#include <vector>

template <class T> struct NoOverflow {
    NoOverflow() = default;
    NoOverflow(T value) : a(value) {}
    NoOverflow operator+(NoOverflow b) const
    {
        if (a > 0 && b > std::numeric_limits<T>::max() - a) {
            // Addition will overflow, return maximum value
            return std::numeric_limits<int>::max();
        } else if (a < 0 && b < std::numeric_limits<T>::min() - a) {
            // Addition will underflow, return minimum value
            return std::numeric_limits<T>::min();
        } else {
            // No overflow, perform addition
            return a + b;
        }
    }
    NoOverflow operator-(NoOverflow b) const
    {
        if (b > 0 && a < std::numeric_limits<T>::min() + b) {
            return std::numeric_limits<T>::min();
        } else if (b < 0 && a > std::numeric_limits<int>::max() + b) {
            return std::numeric_limits<int>::max();
        } else {
            return a - b;
        }
    }
    NoOverflow &operator+=(NoOverflow b)
    {
        *this = *this + b;
        return *this;
    }
    NoOverflow &operator-=(NoOverflow b)
    {
        *this = *this - b;
        return *this;
    }

    operator T() const { return a; }

  private:
    T a{};
};

struct Size {
    Size() = default;
    Size(std::size_t width, std::size_t height) : minWidth(width), maxWidth(width), minHeight(height), maxHeight(height)
    {
    }
    Size(std::size_t minWidth, std::size_t minHeight, std::size_t maxWidth, std::size_t maxHeight)
        : minWidth(minWidth), minHeight(minHeight), maxWidth(maxWidth), maxHeight(maxHeight)
    {
    }
    NoOverflow<std::size_t> minWidth{};
    NoOverflow<std::size_t> minHeight{};
    NoOverflow<std::size_t> maxWidth{};
    NoOverflow<std::size_t> maxHeight{};
};

#include <iostream>
class View
{
  public:
    View(std::size_t width, std::size_t height) : width(width), height(height), x(0), y(0) { set(0, 0); }
    View(const View &parent, std::size_t xOffset, std::size_t yOffset, std::size_t width, std::size_t height)
        : width(width), height(height), x(parent.x + xOffset), y(parent.y + yOffset)
    {
        set(0, 0);
    }

    void clear();
    void set(std::size_t column, std::size_t row) { std::cout << ANSIControlCodes::moveCursorTo(x + column, y + row); }

    void write(std::string_view data) { std::cout << data; }

    const std::size_t width;
    const std::size_t height;

  private:
    const std::size_t x;
    const std::size_t y;
};

class BaseElementImpl
{
  public:
    virtual void render(View &view) = 0;
    virtual Size getSize() const = 0;
    virtual bool focusable() const { return false; }
    void setFocus(bool focus) { focused = focus; }
    bool isFocused() const { return focused; }

  private:
    bool focused;
};
using BaseElement = std::shared_ptr<BaseElementImpl>;

struct DecoratorImpl : BaseElementImpl {
    BaseElement inner;
    DecoratorImpl(BaseElement inner) : inner(std::move(inner)) {}
};

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
            View subview{view, view.width / 2 - inner->getSize().minWidth / 2, 0, inner->getSize().minWidth,
                         inner->getSize().minHeight};
            inner->render(subview);
        }
        Size getSize() const override { return inner->getSize(); };
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
                view.write(color);
                inner->render(view);
                view.write(ANSIControlCodes::RESET);
            }
            Size getSize() const override { return inner->getSize(); };

          private:
            std::string_view color;
        };
        return Element<Impl>(std::make_shared<Impl>(inner, color));
    };
}

auto Text(const std::string &other)
{
    class Impl : public BaseElementImpl
    {
      public:
        Impl(std::string text) : text(text) {}
        void setText(const std::string &newtext) { text = newtext; }

        void render(View &view) override { view.write(text); }

        Size getSize() const override { return {text.size(), 1}; }

      private:
        std::string text;
    };

    return Element<Impl>(other);
}

template <typename T> auto Button(const std::string &label, T action)
{
    using TextImpl = decltype(Text(""))::Type;
    class Impl : public TextImpl
    {
      public:
        Impl(std::string text) : TextImpl("[ " + text + " ]") {}
        bool focusable() const override { return true; }
        void render(View &view) override
        {
            if (isFocused()) {
                view.write(ANSIControlCodes::INVERT);
                TextImpl::render(view);
                view.write(ANSIControlCodes::RESET);
            } else {
                TextImpl::render(view);
            }
        }
    };
    return Element<Impl>(label);
}

template <class... Elements> auto VContainer(Elements... elements)
{
    class Impl : public BaseElementImpl
    {
      public:
        Impl(std::vector<BaseElement> elements) : elements(std::move(elements)) {}

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
                height = std::min<std::size_t>(height, view.height - offset);
                View subview{view, 0, offset, view.width, height};
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
            }
            return size;
        }

      private:
        std::vector<BaseElement> elements;
    };
    return Element<Impl>(std::make_shared<Impl>(std::vector<BaseElement>{elements...}));
}

auto Bottom(BaseElement inner)
{
    class Impl : public DecoratorImpl
    {
      public:
        using DecoratorImpl::DecoratorImpl;
        void render(View &view) override
        {
            View subview{view, 0, view.height - inner->getSize().minHeight, inner->getSize().minWidth,
                         inner->getSize().minHeight};
            inner->render(subview);
        }
        Size getSize() const override { return inner->getSize(); };
    };
    return Element<Impl>(std::make_shared<Impl>(inner));
}

auto flex(std::size_t maxWidth = std::numeric_limits<std::size_t>::max(),
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

auto xflex(std::size_t maxWidth = std::numeric_limits<std::size_t>::max())
{
    return flex(maxWidth, std::numeric_limits<std::size_t>::max());
}
auto yflex(std::size_t maxHeight = std::numeric_limits<std::size_t>::max())
{
    return flex(std::numeric_limits<std::size_t>::max(), maxHeight);
}

#if 0


class Bottom : public Decorator<Bottom>
{
  public:
    using Decorator::Decorator;
    void render(View &view) override
    {
        View subview{view, 0, view.height - inner->getSize().height, inner->getSize().width, inner->getSize().height};
        inner->render(subview);
    }
    Size getSize() const override { return inner->getSize(); };
};

#endif