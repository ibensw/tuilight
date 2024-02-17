#include "element.h"
#include "terminal.h"
#include <array>
#include <string>

int main()
{
    Terminal t;

    std::vector<BaseElement> elements;
    for (int i = 0; i < 50; ++i) {
        elements.push_back(Button("Test " + std::to_string(i), [] {}));
    }
    auto a = VMenu(elements);

    auto b = Button("Quit", [&] { t.stop(); });
    auto both = VContainer(a | Fit, b);

    t.runInteractive(both);
    t.clear();
}
