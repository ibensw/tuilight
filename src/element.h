#pragma once

#include "ansi.h"
#include <memory>
#include <string_view>
#include <vector>

struct Size {
    std::size_t width;
    std::size_t height;
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
};
using BaseElement = std::shared_ptr<BaseElementImpl>;

struct DecoratorImpl : BaseElementImpl {
    BaseElement inner;
    DecoratorImpl(BaseElement inner) : inner(std::move(inner)) {}
};

template <class D> class Element : public std::shared_ptr<D>
{
  public:
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
            View subview{view, view.width / 2 - inner->getSize().width / 2, 0, inner->getSize().width,
                         inner->getSize().height};
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

template <class... Elements> auto VContainer(Elements... elements)
{
    class Impl : public BaseElementImpl
    {
      public:
        Impl(std::vector<BaseElement> elements) : elements(std::move(elements)) {}

        void render(View &view) override
        {
            std::size_t offset{};
            for (auto &element : elements) {
                View subview{view, 0, offset, view.width, view.height - offset};
                element->render(subview);
                offset += element->getSize().height;
            }
        }

        Size getSize() const override
        {
            std::size_t width = 0;
            std::size_t height = 0;
            for (auto &element : elements) {
                width = std::max(width, element->getSize().width);
                height += element->getSize().height;
            }
            return {width, height};
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
            View subview{view, 0, view.height - inner->getSize().height, inner->getSize().width,
                         inner->getSize().height};
            inner->render(subview);
        }
        Size getSize() const override { return inner->getSize(); };
    };
    return Element<Impl>(std::make_shared<Impl>(inner));
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