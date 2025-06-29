#include "chip8.h"
#include "lib/tinyfiledialogs/tinyfiledialogs.h"

Chip8 emulator;

// Источники инфы:
// - https://multigesture.net/articles/how-to-write-an-emulator-chip-8-interpreter/
// - https://en.wikipedia.org/wiki/CHIP-8
// - ChatGPT :)
int main() {
    emulator.initialize();
    emulator.setupGraphics();

    const char* filters[] = { "*.ch8" };
    const char* file = tinyfd_openFileDialog("Выбрать ROM", "", 1, filters, "CHIP‑8 ROM", 0);
    if (file)
        emulator.loadROM(file);

    while (true) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                SDL_Quit();
                return 0;
            }
        }

        emulator.emulateCycle();
        emulator.renderGraphics();
    }
}
