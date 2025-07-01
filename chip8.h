#ifndef CHIP8_H
#define CHIP8_H
#include <SDL2/SDL.h>
#include <cstdint>

class Chip8 {
    uint16_t opcode = 0;
    uint8_t memory[4 * 1024] = {}; // Память 4Кб
    uint8_t V[16] = {}; // Регистры V0-VF
    uint16_t index = 0;
    uint16_t program_counter = 0x200;
    uint16_t stack[16] = {};
    uint16_t stack_pointer = 0;
    uint8_t gfx[64 * 32] = {}; // Графический буфер
    uint8_t delay_timer = 0;
    uint8_t sound_timer = 0;
    uint8_t key[16] = {};

    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_Texture* texture = nullptr;

public:
    void initialize();
    void setupGraphics();
    void renderGraphics() const;
    void handleKeyEvent(const SDL_Event& event);
    void loadROM(const char* filename);
    void emulateCycle();
};

#endif //CHIP8_H
