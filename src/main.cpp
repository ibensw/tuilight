#include "element.h"
#include "terminal.h"

int main()
{
    Terminal t;

    auto keyPress = Text::create("No key pressed");
    auto keyColor = Color::create(ANSIControlCodes::FG_RED);

    auto hello = Element(Text::create("Hello world!")) | Color::create(ANSIControlCodes::FG_BLUE) | Center::create();
    auto hello3 = Element(keyPress) | keyColor | Center::create();
    auto hello2 = Element(Text::create("Hello world!")) | Bottom::create() | Color::create(ANSIControlCodes::FG_GREEN);

    auto both = VContainer::create(std::vector<Element>{hello, hello3, hello2});

    while (true) {
        t.clear();
        t.render(both);
        auto key = getchar();
        if (key == 27) {
            std::cout << "Exiting because of " << key << std::endl;
            break;
        }
        keyPress->setText(std::to_string(key));
    }
    // t.clear();
}