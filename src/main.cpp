#include "element.h"
#include "terminal.h"
#include <string>

int main()
{
    Terminal t;

    auto keyPress = Text("No key pressed");
    auto keyColor = Color(ANSIControlCodes::FG_RED);

    auto hello = Text("Hello world!") | Color(ANSIControlCodes::FG_BLUE) | Center;
    auto hello3 = keyPress | keyColor | Center;
    auto quit = Button("Quit", [] {});
    auto hello2 = quit | Color(ANSIControlCodes::FG_GREEN);
    auto manystyles = VContainer(Text("Test1") | Color(ANSIControlCodes::FG_RED) | Underline,
                                 Text("Test2") | Bold | Center, Text("Test3") | Dim);

    std::vector<BaseElement> manyLines;
    for (uint8_t i = 32; i < 255; ++i) {
        std::string entry = "Character " + std::to_string(i) + " = ";
        entry.push_back(static_cast<char>(i));
        manyLines.push_back(Text(entry));
    }
    // manyLines.push_back(Text("ABC"));
    // manyLines.push_back(Text("DEF"));
    // manyLines.push_back(Text("GHI"));
    // manyLines.push_back(Text("JKL"));
    // manyLines.push_back(Text("MNO"));
    // manyLines.push_back(Text("PQR"));
    // manyLines.push_back(Text("STU"));
    // manyLines.push_back(Text("VWX"));
    // manyLines.push_back(Text("YZ"));
    // manyLines.push_back(Text("ABC"));
    // manyLines.push_back(Text("DEF"));
    // manyLines.push_back(Text("GHI"));
    // manyLines.push_back(Text("JKL"));
    // manyLines.push_back(Text("MNO"));
    // manyLines.push_back(Text("PQR"));
    // manyLines.push_back(Text("STU"));
    // manyLines.push_back(Text("VWX"));
    // manyLines.push_back(Text("YZ"));
    // for (auto c : "Hello world, this is me!") {
    //     manyLines.push_back(Text(std::string(5, c)));
    // }
    auto manyLines2 = VContainer(manyLines);

    std::size_t scroll = 0;

    auto both = VContainer(hello, hello3, manyLines2 | vScroll(10, &scroll, true), manystyles | yStretch(), hello2);

    while (true) {
        t.clear();
        t.render(both);
        auto key = getchar();
        if (key == 27) {
            break;
        }
        quit->setFocus(!quit->isFocused());
        keyPress->text += " " + std::to_string(key);
        // keyPress->setText(std::to_string(key));
        scroll++;
        scroll %= manyLines2->getSize().minHeight;
    }
    t.clear();
}