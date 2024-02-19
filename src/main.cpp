#include "element.h"
#include "terminal.h"
#include <array>
#include <string>
#include <thread>

int main()
{
    Terminal t;

    std::vector<BaseElement> elements;
    for (int i = 0; i < 50; ++i) {
        elements.push_back(Button("Test " + std::to_string(i), [] {}));
    }
    auto a = VMenu(elements);

    std::atomic<bool> running = true;
    std::thread scroller([&]() {
        while (running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            t.post([&](Terminal &, BaseElement) { a->next(); });
        }
    });

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
    auto both = VContainer(a | Fit, b);

    t.runInteractive(both | KeyHander(handle));
    running = false;
    scroller.join();
    t.clear();
}
