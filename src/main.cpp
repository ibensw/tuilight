#include "element.h"
#include "terminal.h"
#include <array>
#include <string>

int main()
{
    Terminal t;

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
    manyLines.push_back(Text("Short lkine"));
    manyLines.push_back(Text("This is a very long line that is longer than 20 chars"));
    for (uint8_t i = 32; i < 255; ++i) {
        std::string entry = "Character " + std::to_string(i) + " = ";
        entry.push_back(static_cast<char>(i));
        manyLines.push_back(Text(entry) | colors.at(i % 2));
    }
    auto manyLines2 = VContainer(manyLines);

    std::size_t scroll = 0;
    auto noquit = Button("Scroll down", [&] { scroll++; });

    auto both = VContainer(hello, hello3, manyLines2 | vScroll(5, &scroll, true) | Frame | Limit(30),
                           manystyles | yStretch(), hello2, noquit);

    t.runInteractive(both);
    t.clear();
}