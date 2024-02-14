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

class BaseElement
{
  public:
    virtual void render(View &view) = 0;
    virtual Size getSize() const = 0;
};

class Element : public std::shared_ptr<BaseElement>
{
  public:
    using std::shared_ptr<BaseElement>::shared_ptr;
    // template <typename T, class... ArgTypes> Element decorate(ArgTypes... args) { return T::create(*this, args...); }
    template <typename T> Element operator|(T tmplt) { return tmplt(*this); }
};

template <class D> class DerivedElement : public std::shared_ptr<D>
{
  public:
    // using std::shared_ptr<D>::shared_ptr;
    DerivedElement(std::shared_ptr<D> &&other) : std::shared_ptr<D>(std::move(other)) {}
    // template <typename T, class... ArgTypes> Element decorate(ArgTypes... args) { return T::create(*this, args...); }
    template <typename T> Element operator|(T tmplt) { return tmplt(*this); }
};

template <class E> class Creatable
{
  public:
    template <class... Types> static DerivedElement<E> create(Types... args) { return std::make_shared<E>(args...); }
};

// class Decorator : public BaseElement
// {
//   public:
//     Decorator(Element element) : inner(element) {}

//   protected:
//     Element inner;
// };

template <class T> class Decorator : public BaseElement, private Creatable<T>
{
  public:
    Decorator(Element element) : inner(element) {}
    template <class... Types> static auto create(Types... args)
    {
        return [args...](Element inner) { return Creatable<T>::create(inner, args...); };
    }

  protected:
    Element inner;
};

#include <string>
class Text : public BaseElement, public Creatable<Text>
{
  public:
    Text(std::string text) : text(text) {}
    void setText(const std::string &newtext) { text = newtext; }

    void render(View &view) override
    {
        // view.clear();
        view.write(text);
    }

    Size getSize() const override { return {text.size(), 1}; }

  private:
    std::string text;
};

class Center : public Decorator<Center>
{
  public:
    using Decorator::Decorator;
    void render(View &view) override
    {
        View subview{view, view.width / 2 - inner->getSize().width / 2, 0, inner->getSize().width,
                     inner->getSize().height};
        inner->render(subview);
    }
    Size getSize() const override { return inner->getSize(); };
};

class Color : public Decorator<Color>
{
  public:
    Color(Element inner, std::string_view color) : Decorator(inner), color(color) {}
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

class VContainer : public BaseElement, public Creatable<VContainer>
{
  public:
    VContainer(std::vector<Element> elements) : elements(elements) {}
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
    std::vector<Element> elements;
};

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
