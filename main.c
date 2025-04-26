#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <SDL2/SDL.h>
#include <z80.h> // Make sure this points to your libz80 header

#define TEXTURE_WIDTH 256
#define TEXTURE_HEIGHT 256
#define SCALE 3
#define RAM_SIZE 0x10000 // 64KB max addressable by Z80

uint8_t memory[RAM_SIZE];
Z80Context cpu;

static byte context_mem_read_callback(size_t param, ushort address) {
    byte data = memory[address];
    //printf("memR %04x %02x\n", address, data);
    return data;
}

static void context_mem_write_callback(size_t param, ushort address, byte data) {
    //printf("memW %04x %02x\n", address, data);
    memory[address] = data;
}

static byte context_io_read_callback(size_t param, ushort address) {
  byte data = address >> 8;
  printf("ioR %04x %02x\n", address, data);
  return data;
}

static void context_io_write_callback(size_t param, ushort address, byte data) {
  printf("ioW %04x %02x\n", address, data);
}

int load_rom(const char* filename,ushort address) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        perror("Failed to open ROM:");
        perror(filename);
        return 0;
    }
    
    size_t bytesRead = fread(memory+address, 1, RAM_SIZE, file);
    fclose(file);
    printf("Loaded %s 0x%04lX bytes into address 0x%04X.\n", filename, bytesRead,address);
    return 1;
}

int main(int argc, char* argv[]) {


    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return 1;
    }

    // Create a window
    SDL_Window* window = SDL_CreateWindow("Byte Array Texture", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          TEXTURE_WIDTH*SCALE, TEXTURE_HEIGHT*SCALE, SDL_WINDOW_SHOWN);
    if (window == NULL) {
        fprintf(stderr, "Window could not be created! SDL_Error: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    // Create a renderer
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == NULL) {
        fprintf(stderr, "Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }




    // Load roms
    if (!load_rom("CPM/CPMjump.bin",0x0000)) {
        return 1;
    }
    if (!load_rom("CPM/CPM22.bin",0xdc00)) {
        return 1;
    }

    // Initialize Z80 CPU
    Z80RESET(&cpu);

    cpu.memRead = context_mem_read_callback;
    cpu.memWrite = context_mem_write_callback;
    cpu.ioRead = context_io_read_callback;
    cpu.ioWrite = context_io_write_callback;

    // Create a texture
    SDL_Texture* texture = SDL_CreateTexture(renderer, 
        SDL_PIXELFORMAT_RGBA8888, 
        SDL_TEXTUREACCESS_STREAMING, 
        TEXTURE_WIDTH, 
        TEXTURE_HEIGHT);

    if (texture == NULL) {
        fprintf(stderr, "Texture could not be created! SDL_Error: %s\n", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }


    // Keep the window open until the user closes it
    SDL_Event event;
    int quit = 0;
    while (!quit) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                quit = 1;
            }
        }
        if (cpu.halted) {
            quit = 1;
        }
        // Update the texture with the byte array data. This is necessary as CreateTextureFromSurface is deprecated.
        SDL_UpdateTexture(texture, NULL, memory, TEXTURE_WIDTH); // RGBA8888 is 4 bytes per pixel
        SDL_Rect dest_rect = { 0, 0, TEXTURE_WIDTH, TEXTURE_HEIGHT };
        // Render the texture to the screen
        SDL_RenderCopy(renderer, texture, NULL, &dest_rect);
        SDL_RenderPresent(renderer);
        //SDL_UpdateWindowSurface(window);
        //SDL_Delay(5);
        //printf("PC: 0x%04X\n", cpu.PC);
        Z80Execute(&cpu);

    }

    // Clean up
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}


/*
    // Allocate memory for the byte array
    unsigned char* byteArray = (unsigned char*)malloc(BYTE_ARRAY_SIZE);
    if (byteArray == NULL) {
        fprintf(stderr, "Memory allocation failed! SDL_Error: %s\n", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Fill the byte array with some data.  Replace this with your actual data source.
    for (int i = 0; i < BYTE_ARRAY_SIZE; ++i) {
        byteArray[i] = (unsigned char)(rand() % 256);  // Example: random bytes
    }

*/