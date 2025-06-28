#include <iostream>

#include "chip8.h"

Chip8 emulator;

// Источники инфы:
// - https://multigesture.net/articles/how-to-write-an-emulator-chip-8-interpreter/
// - https://en.wikipedia.org/wiki/CHIP-8
// - ChatGPT :)
[[noreturn]] int main() {
    emulator.setupGraphics();

    emulator.initialize();
    emulator.loadROM("/home/asus/Загрузки/test_opcode.ch8");

    while (true) {
        emulator.emulateCycle();

        emulator.renderGraphics();
    }
}
