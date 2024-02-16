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
    virtual void render(View &view) = 0;
    virtual ElementSize getSize() const = 0;
    virtual bool focusable() const { return false; }
    virtual void setFocus(bool focus) { focused = focus; }
    bool isFocused() const { return focused; }
    virtual bool handleEvent(int event) { return false; }
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
    bool handleEvent(int event) override;

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
    bool handleEvent(int event) override;
    BaseElement focusedChild() const { return focusableChildren.at(focusedElement); }

    std::vector<BaseElement> elements;
    std::vector<BaseElement> focusableChildren;
    std::size_t focusedElement{};
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
    bool handleEvent(int event) override;
    BaseElement focusedChild() const { return elements.at(focusedIndex); }

  private:
    std::vector<BaseElement> elements;
    std::size_t focusedIndex{};
    std::size_t scrolledValue{};
    mutable std::vector<long> offsets;
};

struct NoEscape : DecoratorImpl {
    using DecoratorImpl::DecoratorImpl;
    bool handleEvent(int event) override;
};

// auto makeStyle(void (*modifier)(Style &))
// {
//     return [modifier](BaseElement inner) { return Element<Styler>(inner, modifier); };
// }

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

// auto ScrollBox(std::size_t width = std::numeric_limits<std::size_t>::max(),
//                std::size_t height = std::numeric_limits<std::size_t>::max(), std::size_t *xOffset = nullptr,
//                std::size_t *yOffset = nullptr, bool xScrollbar = false, bool yScrollbar = false)
// {
//     return [=](BaseElement inner) {
//         class Impl : public DecoratorImpl
//         {
//           public:
//             Impl(BaseElement inner, std::size_t width, std::size_t height, std::size_t *xOffset, std::size_t
//             *yOffset,
//                  bool xScrollbar, bool yScrollbar)
//                 : DecoratorImpl(inner), width(width), height(height), xOffset(xOffset), yOffset(yOffset),
//                   xScrollbar(xScrollbar), yScrollbar(yScrollbar)
//             {
//             }
//             void render(View &view) override
//             {
//                 class ScrollView : public SubView
//                 {
//                   public:
//                     ScrollView(View &view, std::size_t width, std::size_t height, std::size_t xScroll,
//                                std::size_t yScroll)
//                         : SubView(view, 0, 0, width, height), xScroll(xScroll), yScroll(yScroll)
//                     {
//                     }
//                     std::size_t xScroll;
//                     std::size_t yScroll;

//                     void write(std::size_t column, std::size_t row, Style style, std::string_view data) override
//                     {
//                         SubView::write(column - xScroll, row - yScroll, style, data);
//                     }
//                 };

//                 auto viewWidth = view.width;
//                 auto innerHeight = inner->getSize().minHeight;
//                 if (yOffset && yScrollbar && innerHeight > view.height) {
//                     auto maxScroll = innerHeight - view.height;
//                     if (*yOffset > maxScroll) {
//                         *yOffset = maxScroll;
//                     }
//                     auto scrollBarSize = std::max<std::size_t>(1, view.height * view.height / innerHeight);
//                     auto scrollOffset = ((view.height - scrollBarSize) * *yOffset + maxScroll / 2) / (maxScroll);
//                     while (scrollBarSize > 0) {
//                         view.write(view.width - 1, scrollOffset, view.viewStyle, "#");
//                         --scrollBarSize;
//                         ++scrollOffset;
//                     }
//                     viewWidth--;
//                 }
//                 ScrollView subview(view, viewWidth, view.height, xOffset ? *xOffset : 0, yOffset ? *yOffset : 0);
//                 inner->render(subview);
//             }
//             ElementSize getSize() const override
//             {
//                 auto size = inner->getSize();
//                 if (yScrollbar) {
//                     size.minWidth++;
//                     size.maxWidth = std::max(size.maxWidth, size.maxWidth + 1);
//                 }
//                 size.minWidth = std::min<std::size_t>(size.minWidth, width);
//                 size.minHeight = std::min<std::size_t>(size.minHeight, height);
//                 size.maxWidth = std::min<std::size_t>(size.maxWidth, width);
//                 size.maxHeight = std::min<std::size_t>(size.maxHeight, height);
//                 return size;
//             };

//           private:
//             std::size_t width;
//             std::size_t height;
//             std::size_t *xOffset = nullptr;
//             std::size_t *yOffset = nullptr;
//             bool xScrollbar = false;
//             bool yScrollbar = false;
//         };
//         return Element<Impl>(std::make_shared<Impl>(inner, width, height, xOffset, yOffset, xScrollbar, yScrollbar));
//     };
// }

// auto vScroll = [](std::size_t height, std::size_t *yOffset = nullptr, bool scrollbar = false) {
//     return ScrollBox(std::numeric_limits<std::size_t>::max(), height, nullptr, yOffset, false, scrollbar);
// };

inline auto Frame(BaseElement inner) { return Element<detail::Frame>(inner); }

inline auto Selectable(BaseElement inner) { return Element<detail::Selectable>(inner); }

inline auto VMenu(const std::vector<BaseElement> &elements) { return Element<detail::VMenu>(elements); }

inline auto NoEscape(BaseElement inner) { return Element<detail::NoEscape>(inner); }