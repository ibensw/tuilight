#include "tuilight/element.h"
#include "tuilight/terminal.h"
#include <array>
#include <string>
#include <thread>

using namespace wibens::tuilight;

int main()
{
    Terminal t;

    std::vector<BaseElement> elements;
    for (int i = 0; i < 10; ++i) {
        // auto x = Text("left");
        auto x = HContainer(Button("left", [] {}) | HStretch(), Button("right", [] {}));
        elements.push_back(x);
        // elements.push_back(HContainer(Text("left"), Text("right")));
    }
    auto a = VMenu(elements);

    std::atomic<bool> running = true;

    auto handle = [&](KeyEvent e, BaseElement be) {
        if (be->handleEvent(e)) {
            return true;
        }
        if (e == KeyEvent::ESCAPE) {
            t.stop();
            return true;
        }
        return false;
    };

    auto b = Button("Quit", [&] { t.stop(); });
    auto both = VContainer(a | Fit | ForegroundColor(Color::Gray), b);

    t.runInteractive(both | KeyHander(handle));
    running = false;
    t.clear();
}
