#include "element.h"
#include "terminal.h"

int main()
{
    Terminal t;

    auto keyPress = Text("No key pressed");
    auto keyColor = Color(ANSIControlCodes::FG_RED);

    auto hello = Text("Hello world!") | Color(ANSIControlCodes::FG_BLUE) | Center;
    auto hello3 = keyPress | keyColor | Center;
    auto quit = Button("Quit", [] {});
    auto hello2 = quit | Color(ANSIControlCodes::FG_GREEN);
    auto manystyles = VContainer(Text("Test1") | Color(ANSIControlCodes::FG_RED) | Underline, Text("Test2") | Bold,
                                 Text("Test3") | Dim);

    auto both = VContainer(hello, hello3 | yflex(), manystyles, hello2);

    while (true) {
        t.clear();
        t.render(both);
        auto key = getchar();
        if (key == 27) {
            break;
        }
        quit->setFocus(!quit->isFocused());
        keyPress->setText(std::to_string(key));
    }
    t.clear();
}