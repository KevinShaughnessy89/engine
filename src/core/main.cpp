#define TRACY_ENABLE

#include <mimalloc-new-delete.h>

#include <ios>

#include "Game.hpp"
#include "core/Core.hpp"
#include "core/InputHandler.hpp"
#include "core/PrecompiledHeader.hpp"

int main(int argc, char** argv) {
    std::ios::sync_with_stdio(false);
    Game game;
    Input.addKeyObserver(&game);
    game.init();
    game.run();

    return 0;
}
