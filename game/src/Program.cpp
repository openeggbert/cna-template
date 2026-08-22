#include "HelloGame/HelloGame.hpp"

#include <iostream>
#include <string_view>

int main(int argc, char* argv[])
{
    const bool smokeTest = argc == 2 && std::string_view(argv[1]) == "--smoke-test";

    HelloGame game(smokeTest);
    game.Run();
    if (smokeTest)
        std::cout << "cna-template: Game::Run returned; destroying game\n";

    return 0;
}
