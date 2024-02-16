#include "element.h"
#include "terminal.h"
#include <array>
#include <string>

int main()
{
    Terminal t;
    // while (true) {
    //     // std::cout << std::hex << getchar() << std::endl;
    //     std::cout << static_cast<int>(Terminal::keyPress()) << std::endl;
    // }
    // return 0;

    struct UnderlineButton : detail::Button {
        using detail::Button::Button;
        void render(View &view) override
        {
            if (isFocused()) {
                Element<detail::Text> copy(*this);
                Underline(copy)->render(view);
            } else {
                Text::render(view);
            }
        }
    };

    auto keyPress = Text("No key pressed");
    auto keyColor = Color(ANSIControlCodes::FG_RED);

    auto hello = Text("Hello world!") | Color(ANSIControlCodes::FG_BLUE) | Center;
    auto hello3 = keyPress | keyColor | Center;
    auto quit = Button("Quit", [&] { t.stop(); });
    auto hello2 = quit | Color(ANSIControlCodes::FG_GREEN);
    auto manystyles = VContainer(Text("Test1") | Color(ANSIControlCodes::FG_RED) | Underline,
                                 Text("Test2") | Bold | Center, Text("Test3") | Dim);

    std::vector<BaseElement> manyLines;

    auto green = Color(ANSIControlCodes::FG_GREEN);
    auto red = Color(ANSIControlCodes::FG_RED);
    std::array colors{green, red};
    // manyLines.push_back(Text("Short lkine", true) | HStretch() | Selectable);
    // manyLines.push_back(Text("This is a very long line that is longer than 20 chars", true) | HStretch() |
    // Selectable);
    for (uint8_t i = 32; i < 255; ++i) {
        std::string entry = "Character " + std::to_string(i) + " = ";
        entry.push_back(static_cast<char>(i));
        manyLines.push_back(Text(entry, true) | HStretch() | colors.at(i % 2) | Selectable);
    }
    auto manyLines2 = VMenu(manyLines);

    std::size_t scroll = 0;

    auto both = VContainer(hello, hello3, manyLines2 | Limit(30, 5), manystyles | VStretch(), hello2);

    t.runInteractive(both);
    t.clear();
}