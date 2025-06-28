#include "chip8.h"

#include <iostream>

void Chip8::initialize() {
    opcode = 0;
    index = 0;
    program_counter = 0x200;
    stack_pointer = 0;
    delay_timer = 0;
    sound_timer = 0;
    window = nullptr;
    renderer = nullptr;
    texture = nullptr;

    memset(memory, 0, sizeof(memory));
    memset(V, 0, sizeof(V));
    memset(stack, 0, sizeof(stack));
    memset(gfx, 0, sizeof(gfx));
    memset(key, 0, sizeof(key));

    const uint8_t chip8_fontset[80] =
    {
        0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
        0x20, 0x60, 0x20, 0x20, 0x70, // 1
        0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
        0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
        0x90, 0x90, 0xF0, 0x10, 0x10, // 4
        0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
        0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
        0xF0, 0x10, 0x20, 0x40, 0x40, // 7
        0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
        0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
        0xF0, 0x90, 0xF0, 0x90, 0x90, // A
        0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
        0xF0, 0x80, 0x80, 0x80, 0xF0, // C
        0xE0, 0x90, 0x90, 0x90, 0xE0, // D
        0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
        0xF0, 0x80, 0xF0, 0x80, 0x80 // F
    };
    for (int i = 0; i < 80; i++) {
        memory[i] = chip8_fontset[i];
    }
}

void Chip8::setupGraphics() {
    SDL_Init(SDL_INIT_VIDEO);
    window = SDL_CreateWindow("CHIP-8", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 320, 0);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, 64, 32);
}

void Chip8::renderGraphics() {
    uint32_t pixels[64 * 32];
    for (int i = 0; i < sizeof(gfx); ++i)
        pixels[i] = gfx[i] ? 0xFFFFFFFF : 0x00000000;

    SDL_UpdateTexture(texture, nullptr, pixels, 64 * sizeof(uint32_t));
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, nullptr, nullptr);
    SDL_RenderPresent(renderer);

    SDL_Delay(16); // ~60 FPS
}

void Chip8::loadROM(const char *filename) {
    FILE *rom = fopen(filename, "rb");
    if (!rom) {
        std::cout << "Failed to open file " << filename << std::endl;
        exit(1);
    }

    const size_t bytesRead = fread(memory + 0x200, 1, sizeof(this->memory) - 0x200, rom);
    std::cout << "Loaded file " << filename << " (Bytes read: " << bytesRead << ")" << std::endl;
    fclose(rom);
}

void Chip8::emulateCycle() {
    opcode = memory[program_counter] << 8 | memory[program_counter + 1];

    switch (opcode & 0xF000) {
        case 0x0000:
            switch (opcode & 0x00FF) {
                case 0x00E0: // Clears the screen.
                    memset(gfx, 0, sizeof(gfx));
                    program_counter += 2;
                    break;

                case 0x00EE: // Returns from a subroutine.
                    program_counter = stack[stack_pointer];
                    stack_pointer--;
                    program_counter += 2;
                    break;

                default:
                    std::cout << "Unknown opcode: " << std::hex << opcode << std::dec << std::endl;
                    exit(1);
            }
            break;

        case 0x1000: // Jumps to address NNN.
            program_counter = opcode & 0x0FFF;
            break;

        case 0x2000: // Calls subroutine at NNN.
            stack_pointer++;
            stack[stack_pointer] = program_counter;
            program_counter = opcode & 0x0FFF;
            break;

        case 0x3000: // Skips the next instruction if VX equals NN (usually the next instruction is a jump to skip a code block).
            if (V[(opcode & 0x0F00) >> 8] == (opcode & 0x00FF)) {
                program_counter += 4;
            }
            else {
                program_counter += 2;
            }
            break;

        case 0x4000: // Skips the next instruction if VX does not equal NN (usually the next instruction is a jump to skip a code block).
            if (V[(opcode & 0x0F00) >> 8] != (opcode & 0x00FF)) {
                program_counter += 4;
            }
            else {
                program_counter += 2;
            }
            break;

        case 0x5000: // Skips the next instruction if VX equals VY (usually the next instruction is a jump to skip a code block).
            if (V[(opcode & 0x0F00) >> 8] == V[(opcode & 0x00F0) >> 4]) {
                program_counter += 4;
            }
            else {
                program_counter += 2;
            }
            break;

        case 0x6000: // Sets VX to NN.
            V[(opcode & 0x0F00) >> 8] = opcode & 0x00FF;
            program_counter += 2;
            break;

        case 0x7000: // Adds NN to VX (carry flag is not changed).
            V[(opcode & 0x0F00) >> 8] += opcode & 0x00FF;
            program_counter += 2;
            break;

        case 0x8000:
            switch (opcode & 0x000F) {
                case 0x0000: // Sets VX to the value of VY.
                    V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x00F0) >> 4];
                    program_counter += 2;
                    break;

                case 0x0001: // Sets VX to VX or VY. (bitwise OR operation).
                    V[(opcode & 0x0F00) >> 8] |= V[(opcode & 0x00F0) >> 4];
                    program_counter += 2;
                    break;

                case 0x0002: // Sets VX to VX and VY. (bitwise AND operation).
                    V[(opcode & 0x0F00) >> 8] &= V[(opcode & 0x00F0) >> 4];
                    program_counter += 2;
                    break;

                case 0x0003: // Sets VX to VX xor VY.
                    V[(opcode & 0x0F00) >> 8] ^= V[(opcode & 0x00F0) >> 4];
                    program_counter += 2;
                    break;

                // 8XY4

                default:
                    std::cout << "Unknown opcode: " << std::hex << opcode << std::dec << std::endl;
                    exit(1);
            }

        default:
            std::cout << "Unknown opcode: " << std::hex << opcode << std::dec << std::endl;
            exit(1);
    }

    if (delay_timer > 0) {
        --delay_timer;
    }

    if (sound_timer > 0) {
        if (sound_timer == 1) {
            std::cout << '\a';
        }
        --sound_timer;
    }
}
