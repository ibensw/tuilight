#pragma once

#include "ansi.h"
#include "view.h"
#include <algorithm>
#include <functional>
#include <limits>
#include <memory>
#include <stack>
#include <string_view>
#include <vector>

struct ElementSize {
    ElementSize() = default;
    ElementSize(std::size_t width, std::size_t height)
        : minWidth(width), maxWidth(width), minHeight(height), maxHeight(height)
    {
    }
    ElementSize(std::size_t minWidth, std::size_t minHeight, std::size_t maxWidth, std::size_t maxHeight)
        : minWidth(minWidth), minHeight(minHeight), maxWidth(maxWidth), maxHeight(maxHeight)
    {
    }
    std::size_t minWidth{};
    std::size_t minHeight{};
    std::size_t maxWidth{};
    std::size_t maxHeight{};
};

class BaseElementImpl
{
  public:
    virtual ~BaseElementImpl() = default;
    virtual void render(View &view) = 0;
    virtual ElementSize getSize() const = 0;
    virtual bool focusable() const { return false; }
    virtual void setFocus(bool focus) { focused = focus; }
    bool isFocused() const { return focused; }
    virtual bool handleEvent(KeyEvent event) { return false; }
    virtual void focusFirst() { setFocus(true); }
    virtual void focusLast() { setFocus(true); }

  private:
    bool focused = false;
};
using BaseElement = std::shared_ptr<BaseElementImpl>;

struct DecoratorImpl : BaseElementImpl {
    BaseElement inner;
    DecoratorImpl(BaseElement inner) : inner(std::move(inner)) {}
    void render(View &view) override { inner->render(view); }
    ElementSize getSize() const override { return inner->getSize(); };
    bool focusable() const override { return inner->focusable(); }
    bool handleEvent(KeyEvent event) override { return inner->handleEvent(event); }
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

namespace detail
{
struct Center : DecoratorImpl {
    using DecoratorImpl::DecoratorImpl;
    void render(View &view) override;
};

struct Color : DecoratorImpl {
    Color(BaseElement inner, std::string_view color) : DecoratorImpl(inner), color(color) {}
    void render(View &view) override;
    std::string_view color;
};

struct Text : BaseElementImpl {
    Text(std::string text, bool fill = false) : text(text), fill(fill) {}
    void render(View &view) override;
    ElementSize getSize() const override { return {text.size(), 1}; }

    std::string text;
    bool fill;
};

struct Button : Text {
  public:
    Button(std::string text, std::function<void(void)> action) : Text("[ " + text + " ]"), action(action) {}
    inline bool focusable() const override { return true; }
    void render(View &view) override;
    bool handleEvent(KeyEvent event) override;

    std::function<void(void)> action;
};

struct VContainer : BaseElementImpl {
    VContainer(const std::vector<BaseElement> &elements);
    void render(View &view) override;
    ElementSize getSize() const override;
    bool focusable() const override { return !focusableChildren.empty(); }
    void setFocus(bool focus) override
    {
        focusableChildren[focusedElement]->setFocus(focus);
        BaseElementImpl::setFocus(focus);
    }
    void focusChild(std::size_t index);
    bool handleEvent(KeyEvent event) override;
    BaseElement focusedChild() const { return focusableChildren.at(focusedElement); }
    void focusFirst() override { focusChild(0); }
    void focusLast() override { focusChild(focusableChildren.size() - 1); }

    std::vector<BaseElement> elements;
    std::vector<BaseElement> focusableChildren;
    std::size_t focusedElement{};
};

struct HContainer : VContainer {
    HContainer(const std::vector<BaseElement> &elements) : VContainer(elements) {}
    void render(View &view) override;
    ElementSize getSize() const override;
    bool handleEvent(KeyEvent event) override;
};

struct Bottom : DecoratorImpl {
    using DecoratorImpl::DecoratorImpl;
    void render(View &view) override;
};

struct Stretch : DecoratorImpl {
    Stretch(BaseElement inner, std::size_t maxWidth, std::size_t maxHeight)
        : DecoratorImpl(inner), maxWidth(maxWidth), maxHeight(maxHeight)
    {
    }
    ElementSize getSize() const override;

    std::size_t maxWidth;
    std::size_t maxHeight;
};

struct Shrink : DecoratorImpl {
    Shrink(BaseElement inner, std::size_t minWidth, std::size_t minHeight)
        : DecoratorImpl(inner), minWidth(minWidth), minHeight(minHeight)
    {
    }
    ElementSize getSize() const override;

    std::size_t minWidth;
    std::size_t minHeight;
};

struct Limit : DecoratorImpl {
    Limit(BaseElement inner, std::size_t maxWidth, std::size_t maxHeight)
        : DecoratorImpl(inner), maxWidth(maxWidth), maxHeight(maxHeight)
    {
    }
    void render(View &view) override;

    ElementSize getSize() const override;

    std::size_t maxWidth;
    std::size_t maxHeight;
};

using StyleFunc = void (*)(Style &);
struct Styler : DecoratorImpl {
    Styler(BaseElement inner, StyleFunc modifier) : DecoratorImpl(inner), modifier(modifier) {}
    void render(View &view) override;

    StyleFunc modifier;
};

struct Frame : DecoratorImpl {
    using DecoratorImpl::DecoratorImpl;
    void render(View &view) override;
    ElementSize getSize() const override;
};

struct Selectable : DecoratorImpl {
    using DecoratorImpl::DecoratorImpl;

    inline bool focusable() const override { return true; }
    inline ElementSize getSize() const override { return inner->getSize(); };
    void render(View &view) override;
};

struct VMenu : BaseElementImpl {
    VMenu(const std::vector<BaseElement> &elements) : elements(elements) {}

    void render(View &view) override;
    ElementSize getSize() const override;
    virtual bool focusable() const { return !elements.empty(); }
    virtual void setFocus(bool focus)
    {
        elements[focusedIndex]->setFocus(focus);
        BaseElementImpl::setFocus(focus);
    }
    bool next();
    bool prev();
    bool handleEvent(KeyEvent event) override;
    BaseElement focusedChild() const { return elements.at(focusedIndex); }

  private:
    std::vector<BaseElement> elements;
    std::size_t focusedIndex{};
    std::size_t scrolledValue{};
    mutable std::vector<long> offsets;
    std::size_t pageSize = 1;
};

struct NoEscape : DecoratorImpl {
    using DecoratorImpl::DecoratorImpl;
    bool handleEvent(KeyEvent event) override;
};

struct PreRender : DecoratorImpl {
    using Hook = std::function<void(BaseElement, const View &)>;
    PreRender(BaseElement inner, Hook hook) : DecoratorImpl(inner), hook(hook) {}
    void render(View &view) override
    {
        hook(inner, view);
        DecoratorImpl::render(view);
    }

    Hook hook;
};

struct KeyHander : DecoratorImpl {
    using Handler = std::function<bool(KeyEvent, BaseElement)>;
    KeyHander(BaseElement inner, Handler handler) : DecoratorImpl(inner), handler(handler) {}
    bool handleEvent(KeyEvent event) override { return handler(event, inner); }

    Handler handler;
};
}; // namespace detail

// template <typename T, typename... Args> auto make_element(Args... args)
// {
//     return Element<T>(std::make_shared<T>(args...));
// }

// inline auto Center(BaseElement inner) { return make_element<detail::Center>(inner); }
inline auto Center(BaseElement inner) { return Element<detail::Center>(inner); }

inline auto Color(std::string_view color)
{
    return [color](BaseElement inner) { return Element<detail::Color>(inner, color); };
}

inline auto Text(const std::string &text, bool fill = false) { return Element<detail::Text>(text, fill); }

inline auto Button(const std::string &label, std::function<void(void)> action)
{
    return Element<detail::Button>(label, action);
}

inline auto VContainer(const std::vector<BaseElement> &elements) { return Element<detail::VContainer>(elements); }
template <class... Elements> auto VContainer(Elements... elements)
{
    return VContainer(std::vector<BaseElement>{elements...});
}

inline auto HContainer(const std::vector<BaseElement> &elements) { return Element<detail::HContainer>(elements); }
template <class... Elements> auto HContainer(Elements... elements)
{
    return HContainer(std::vector<BaseElement>{elements...});
}

inline auto Bottom(BaseElement inner) { return Element<detail::Bottom>(inner); }

inline auto Stretch(std::size_t maxWidth = std::numeric_limits<std::size_t>::max(),
                    std::size_t maxHeight = std::numeric_limits<std::size_t>::max())
{
    return [=](BaseElement inner) { return Element<detail::Stretch>(inner, maxWidth, maxHeight); };
}

inline auto HStretch(std::size_t maxWidth = std::numeric_limits<std::size_t>::max())
{
    return Stretch(maxWidth, std::numeric_limits<std::size_t>::max());
}
inline auto VStretch(std::size_t maxHeight = std::numeric_limits<std::size_t>::max())
{
    return Stretch(std::numeric_limits<std::size_t>::max(), maxHeight);
}

inline auto Shrink(std::size_t minWidth = 0, std::size_t minHeight = 0)
{
    return [=](BaseElement inner) { return Element<detail::Shrink>(inner, minWidth, minHeight); };
}

inline auto HShrink(std::size_t maxWidth = std::numeric_limits<std::size_t>::max()) { return Shrink(maxWidth, 0); }
inline auto VShrink(std::size_t minHeight = 0) { return Shrink(0, minHeight); }

inline auto Fit(BaseElement inner)
{
    auto stretch = Stretch()(inner);
    return Element<detail::Shrink>(inner, 0, 0);
}

inline auto Limit(std::size_t maxWidth = std::numeric_limits<std::size_t>::max(),
                  std::size_t maxHeight = std::numeric_limits<std::size_t>::max())
{
    return [=](BaseElement inner) { return Element<detail::Limit>(inner, maxWidth, maxHeight); };
}

inline auto Bold(BaseElement inner)
{
    return Element<detail::Styler>(inner, [](Style &s) { s.bold = true; });
}
inline auto Dim(BaseElement inner)
{
    return Element<detail::Styler>(inner, [](Style &s) { s.dim = true; });
}
inline auto Underline(BaseElement inner)
{
    return Element<detail::Styler>(inner, [](Style &s) { s.underline = true; });
}
inline auto Invert(BaseElement inner)
{
    return Element<detail::Styler>(inner, [](Style &s) { s.invert = !s.invert; });
}

inline auto Frame(BaseElement inner) { return Element<detail::Frame>(inner); }

inline auto Selectable(BaseElement inner) { return Element<detail::Selectable>(inner); }

inline auto VMenu(const std::vector<BaseElement> &elements) { return Element<detail::VMenu>(elements); }

inline auto NoEscape(BaseElement inner) { return Element<detail::NoEscape>(inner); }

inline auto PreRender(detail::PreRender::Hook hook)
{
    return [=](BaseElement inner) { return Element<detail::PreRender>(inner, hook); };
}

inline auto KeyHander(detail::KeyHander::Handler handler)
{
    return [=](BaseElement inner) { return Element<detail::KeyHander>(inner, handler); };
};
