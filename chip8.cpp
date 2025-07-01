#include "chip8.h"

#include <iostream>

#include "lib/tinyfiledialogs/tinyfiledialogs.h"

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
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    window = SDL_CreateWindow("CHIP-8", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 320, 0);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, 64, 32);
}

void Chip8::renderGraphics() const {
    uint32_t pixels[64 * 32];
    for (int i = 0; i < 64 * 32; ++i)
        pixels[i] = gfx[i] ? 0xFFFFFFFF : 0x00000000;

    SDL_UpdateTexture(texture, nullptr, pixels, 64 * sizeof(uint32_t));
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, nullptr, nullptr);
    SDL_RenderPresent(renderer);
}

void Chip8::handleKeyEvent(const SDL_Event &event) {
    const bool pressed = (event.type == SDL_KEYDOWN);
    switch (event.key.keysym.sym) {
        case SDLK_1: key[0x1] = pressed;
            break;
        case SDLK_2: key[0x2] = pressed;
            break;
        case SDLK_3: key[0x3] = pressed;
            break;
        case SDLK_4: key[0xC] = pressed;
            break;

        case SDLK_q: key[0x4] = pressed;
            break;
        case SDLK_w: key[0x5] = pressed;
            break;
        case SDLK_e: key[0x6] = pressed;
            break;
        case SDLK_r: key[0xD] = pressed;
            break;

        case SDLK_a: key[0x7] = pressed;
            break;
        case SDLK_s: key[0x8] = pressed;
            break;
        case SDLK_d: key[0x9] = pressed;
            break;
        case SDLK_f: key[0xE] = pressed;
            break;

        case SDLK_z: key[0xA] = pressed;
            break;
        case SDLK_x: key[0x0] = pressed;
            break;
        case SDLK_c: key[0xB] = pressed;
            break;
        case SDLK_v: key[0xF] = pressed;
            break;
        default:
            break;
    }
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
        case 0x0000: {
            switch (opcode & 0x00FF) {
                case 0x00E0: {
                    // Clears the screen.
                    memset(gfx, 0, sizeof(gfx));
                    program_counter += 2;
                    break;
                }

                case 0x00EE: {
                    // Returns from a subroutine.
                    program_counter = stack[stack_pointer];
                    stack_pointer--;
                    program_counter += 2;
                    break;
                }

                default:
                    std::cout << "Unknown opcode: " << std::hex << opcode << std::dec << std::endl;
                    exit(1);
            }
            break;
        }

        case 0x1000: {
            // Jumps to address NNN.
            program_counter = opcode & 0x0FFF;
            break;
        }

        case 0x2000: {
            // Calls subroutine at NNN.
            stack_pointer++;
            stack[stack_pointer] = program_counter;
            program_counter = opcode & 0x0FFF;
            break;
        }

        case 0x3000: {
            // Skips the next instruction if VX equals NN (usually the next instruction is a jump to skip a code block).
            if (V[(opcode & 0x0F00) >> 8] == (opcode & 0x00FF)) {
                program_counter += 4;
            } else {
                program_counter += 2;
            }
            break;
        }

        case 0x4000: {
            // Skips the next instruction if VX does not equal NN (usually the next instruction is a jump to skip a code block).
            if (V[(opcode & 0x0F00) >> 8] != (opcode & 0x00FF)) {
                program_counter += 4;
            } else {
                program_counter += 2;
            }
            break;
        }

        case 0x5000: {
            // Skips the next instruction if VX equals VY (usually the next instruction is a jump to skip a code block).
            if (V[(opcode & 0x0F00) >> 8] == V[(opcode & 0x00F0) >> 4]) {
                program_counter += 4;
            } else {
                program_counter += 2;
            }
            break;
        }

        case 0x6000: {
            // Sets VX to NN.
            V[(opcode & 0x0F00) >> 8] = opcode & 0x00FF;
            program_counter += 2;
            break;
        }

        case 0x7000: {
            // Adds NN to VX (carry flag is not changed).
            V[(opcode & 0x0F00) >> 8] += opcode & 0x00FF;
            program_counter += 2;
            break;
        }

        case 0x8000: {
            switch (opcode & 0x000F) {
                case 0x0000: {
                    // Sets VX to the value of VY.
                    V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x00F0) >> 4];
                    program_counter += 2;
                    break;
                }

                case 0x0001: {
                    // Sets VX to VX or VY. (bitwise OR operation).
                    V[(opcode & 0x0F00) >> 8] |= V[(opcode & 0x00F0) >> 4];
                    program_counter += 2;
                    break;
                }

                case 0x0002: {
                    // Sets VX to VX and VY. (bitwise AND operation).
                    V[(opcode & 0x0F00) >> 8] &= V[(opcode & 0x00F0) >> 4];
                    program_counter += 2;
                    break;
                }

                case 0x0003: {
                    // Sets VX to VX xor VY.
                    V[(opcode & 0x0F00) >> 8] ^= V[(opcode & 0x00F0) >> 4];
                    program_counter += 2;
                    break;
                }

                case 0x0004: {
                    // Adds VY to VX. VF is set to 1 when there's an overflow, and to 0 when there is not.
                    const uint16_t sum = V[(opcode & 0x0F00) >> 8] + V[(opcode & 0x00F0) >> 4];
                    V[0xF] = (sum > 255) ? 1 : 0;
                    V[(opcode & 0x0F00) >> 8] = sum & 0xFF;
                    program_counter += 2;
                    break;
                }

                case 0x0005: {
                    // VY is subtracted from VX. VF is set to 0 when there's an underflow, and 1 when there is not. (i.e. VF set to 1 if VX >= VY and 0 if not).
                    const uint8_t x = (opcode & 0x0F00) >> 8;
                    const uint8_t y = (opcode & 0x00F0) >> 4;
                    V[0xF] = (V[x] >= V[y]) ? 1 : 0;
                    V[x] = (V[x] - V[y]) & 0xFF;
                    program_counter += 2;
                    break;
                }

                case 0x0006: {
                    // Shifts VX to the right by 1, then stores the least significant bit of VX prior to the shift into VF
                    const uint8_t x = (opcode & 0x0F00) >> 8;
                    V[0xF] = V[x] & 0x1;
                    V[x] >>= 1;
                    program_counter += 2;
                    break;
                }

                case 0x0007: {
                    // Sets VX to VY minus VX. VF is set to 0 when there's an underflow, and 1 when there is not. (i.e. VF set to 1 if VY >= VX).
                    const uint8_t x = (opcode & 0x0F00) >> 8;
                    const uint8_t y = (opcode & 0x00F0) >> 4;
                    V[0xF] = (V[y] >= V[x]) ? 1 : 0;
                    V[x] = (V[y] - V[x]) & 0xFF;
                    program_counter += 2;
                    break;
                }

                case 0x000E: {
                    // Shifts VX to the left by 1, then sets VF to 1 if the most significant bit of VX prior to that shift was set, or to 0 if it was unset.
                    const uint8_t x = (opcode & 0x0F00) >> 8;
                    V[0xF] = V[x] & 0x80;
                    V[x] <<= 1;
                    program_counter += 2;
                    break;
                }

                default:
                    std::cout << "Unknown opcode: " << std::hex << opcode << std::dec << std::endl;
                    exit(1);
            }
            break;
        }

        case 0x9000: {
            // Skips the next instruction if VX does not equal VY. (Usually the next instruction is a jump to skip a code block).
            if (V[(opcode & 0x0F00) >> 8] != V[(opcode & 0x00F0) >> 4]) {
                program_counter += 4;
            } else {
                program_counter += 2;
            }
            break;
        }

        case 0xA000: {
            // Sets I to the address NNN.
            index = opcode & 0x0FFF;
            program_counter += 2;
            break;
        }

        case 0xB000: {
            // Jumps to the address NNN plus V0.
            program_counter = (opcode & 0x0FFF) + V[0];
            break;
        }

        case 0xC000: {
            // Sets VX to the result of a bitwise and operation on a random number (Typically: 0 to 255) and NN.
            V[(opcode & 0x0F00) >> 8] = (rand() % 256) & (opcode & 0x00FF);
            program_counter += 2;
            break;
        }

        case 0xD000: {
            // Draws a sprite at coordinate (VX, VY) that has a width of 8 pixels and a height of N pixels. Each row of 8 pixels is read as bit-coded starting from memory location I; I value does not change after the execution of this instruction. As described above, VF is set to 1 if any screen pixels are flipped from set to unset when the sprite is drawn, and to 0 if that does not happen.
            // Это пиздец... (Кусок ГПТ кода, который я не понимаю)

            const uint8_t x = V[(opcode & 0x0F00) >> 8];
            const uint8_t y = V[(opcode & 0x00F0) >> 4];
            const uint8_t height = opcode & 0x000F;

            V[0xF] = 0;

            for (int row = 0; row < height; ++row) {
                const uint8_t sprite_byte = memory[index + row];
                for (int col = 0; col < 8; ++col) {
                    if ((sprite_byte & (0x80 >> col)) != 0) {
                        const int x_pos = (x + col) % 64;
                        const int y_pos = (y + row) % 32;
                        const int index_gfx = x_pos + (y_pos * 64);

                        if (gfx[index_gfx] == 1)
                            V[0xF] = 1;

                        gfx[index_gfx] ^= 1;
                    }
                }
            }

            program_counter += 2;
            break;
        }

        case 0xE000: {
            switch (opcode & 0x00FF) {
                case 0x009E:
                    // Skips the next instruction if the key stored in VX(only consider the lowest nibble) is pressed (usually the next instruction is a jump to skip a code block).
                    if (key[V[(opcode & 0x0F00) >> 8]]) {
                        program_counter += 4;
                    } else {
                        program_counter += 2;
                    }
                    break;

                case 0x00A1:
                    // Skips the next instruction if the key stored in VX(only consider the lowest nibble) is not pressed (usually the next instruction is a jump to skip a code block).[24]
                    if (!key[V[(opcode & 0x0F00) >> 8]]) {
                        program_counter += 4;
                    } else {
                        program_counter += 2;
                    }
                    break;

                default:
                    std::cout << "Unknown opcode: " << std::hex << opcode << std::dec << std::endl;
                    exit(1);
            }
            break;
        }

        case 0xF000: {
            switch (opcode & 0x00FF) {
                case 0x0007: {
                    // Sets VX to the value of the delay timer.
                    V[(opcode & 0x0F00) >> 8] = delay_timer;
                    program_counter += 2;
                    break;
                }

                case 0x000A: {
                    // Потом доделать я ебал пока гпт код
                    // A key press is awaited, and then stored in VX (blocking operation, all instruction halted until next key event, delay and sound timers should continue processing).
                    const uint8_t x = (opcode & 0x0F00) >> 8;
                    bool key_pressed = false;

                    for (uint8_t i = 0; i < 16; ++i) {
                        if (key[i]) {
                            V[x] = i;
                            key_pressed = true;
                            break;
                        }
                    }

                    if (!key_pressed)
                        return;

                    program_counter += 2;
                    break;
                }

                case 0x0015: {
                    // Sets the delay timer to VX.
                    delay_timer = V[(opcode & 0x0F00) >> 8];
                    program_counter += 2;
                    break;
                }

                case 0x0018: {
                    // Sets the sound timer to VX.
                    sound_timer = V[(opcode & 0x0F00) >> 8];
                    program_counter += 2;
                    break;
                }

                case 0x001E: {
                    // Adds VX to I. VF is not affected.
                    index += V[(opcode & 0x0F00) >> 8];
                    program_counter += 2;
                    break;
                }

                case 0x0029: {
                    // Sets I to the location of the sprite for the character in VX(only consider the lowest nibble). Characters 0-F (in hexadecimal) are represented by a 4x5 font.
                    index = V[(opcode & 0x0F00) >> 8] * 5;
                    program_counter += 2;
                    break;
                }

                case 0x0033: {
                    // Stores the binary-coded decimal representation of VX, with the hundreds digit in memory at location in I, the tens digit at location I+1, and the ones digit at location I+2.
                    // Опять гпт код...
                    const uint8_t value = V[(opcode & 0x0F00) >> 8];
                    memory[index] = value / 100;
                    memory[index + 1] = (value / 10) % 10;
                    memory[index + 2] = value % 10;
                    program_counter += 2;
                    break;
                }

                case 0x0055: {
                    // Stores from V0 to VX (including VX) in memory, starting at address I. The offset from I is increased by 1 for each value written, but I itself is left unmodified.
                    const uint8_t x = (opcode & 0x0F00) >> 8;
                    for (uint8_t i = 0; i <= x; ++i) {
                        memory[index + i] = V[i];
                    }
                    program_counter += 2;
                    break;
                }

                case 0x0065: {
                    // Fills V0 to VX (including VX) with values from memory, starting at address I. The offset from I is increased by 1 for each value read, but I itself is left unmodified.
                    const uint8_t x = (opcode & 0x0F00) >> 8;
                    for (uint8_t i = 0; i <= x; ++i) {
                        V[i] = memory[index + i];
                    }
                    program_counter += 2;
                    break;
                }

                default:
                    std::cout << "Unknown opcode: " << std::hex << opcode << std::dec << std::endl;
                    exit(1);
            }
            break;

        default:
            std::cout << "Unknown opcode: " << std::hex << opcode << std::dec << std::endl;
            exit(1);
        }
    }

    if (delay_timer > 0) {
        --delay_timer;
    }

    if (sound_timer > 0) {
        if (sound_timer == 1) {
            SDL_AudioSpec want{}, have{};

            want.freq = 44100;
            want.format = AUDIO_S16SYS;
            want.channels = 1;
            want.samples = 2048;
            want.callback = nullptr;

            const SDL_AudioDeviceID dev = SDL_OpenAudioDevice(nullptr, 0, &want, &have, 0);
            if (!dev) return;

            const int sample_count = (have.freq * 100) / 1000;
            auto *buffer = new int16_t[sample_count];

            for (int i = 0; i < sample_count; ++i) {
                buffer[i] = (i / (have.freq / 440 / 2)) % 2 ? 8000 : -8000;
            }

            SDL_PauseAudioDevice(dev, 0);
            SDL_QueueAudio(dev, buffer, sample_count * sizeof(int16_t));
            SDL_Delay(100);
            SDL_CloseAudioDevice(dev);
            delete[] buffer;
        }
        --sound_timer;
    }

    SDL_Delay(1);
}
