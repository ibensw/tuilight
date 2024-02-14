#include "element.h"
#include "terminal.h"

int main()
{
    Terminal t;

    auto keyPress = Text("No key pressed");
    auto keyColor = Color(ANSIControlCodes::FG_RED);

    auto hello = Text("Hello world!") | Color(ANSIControlCodes::FG_BLUE) | Center;
    auto hello3 = keyPress | keyColor | Center;
    auto hello2 = Text("Hello world!") | Bottom | Color(ANSIControlCodes::FG_GREEN);

    auto both = VContainer(hello, hello3, hello2);

    while (true) {
        t.clear();
        t.render(both);
        auto key = getchar();
        if (key == 27) {
            break;
        }
        keyPress->setText(std::to_string(key));
    }
    t.clear();
}