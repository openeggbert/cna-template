#include "HelloGame/HelloGame.hpp"

#include <memory>
#include <string_view>

int main(int argc, char* argv[])
{
    const bool smokeTest = argc == 2 && std::string_view(argv[1]) == "--smoke-test";

    // Heap-allocated on purpose, and this matters on the web: Emscripten ends
    // main() by throwing a JavaScript 'unwind' value so the browser can drive the
    // main loop. That exception passes through the -fwasm-exceptions cleanup
    // landing pad, which destroys stack locals -- including a stack-allocated
    // Game -- while the main loop still holds a pointer to it. The next frame
    // then faults inside BeginDraw() with "RuntimeError: table index is out of
    // bounds". Keeping the Game on the heap and never freeing it is what CNA
    // itself documents for Emscripten, and it is harmless on every other
    // platform, so the template does it unconditionally rather than #ifdef'ing.
    auto game = std::make_unique<HelloGame>(smokeTest);
    game->Run();

#ifdef __EMSCRIPTEN__
    // The main loop outlives main() here; releasing keeps the Game alive for it.
    (void)game.release();
#endif

    return 0;
}
