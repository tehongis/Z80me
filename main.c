#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <z80.h> // Make sure this points to your libz80 header

#define RAM_SIZE 0x10000 // 64KB max addressable by Z80

uint8_t memory[RAM_SIZE];
Z80Context cpu;

static byte context_mem_read_callback(size_t param, ushort address) {
    byte data = memory[address];
    printf("memR %04x %02x\n", address, data);
    return data;
}

static void context_mem_write_callback(size_t param, ushort address, byte data) {
    printf("memW %04x %02x\n", address, data);
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

    printf("Starting emulation...\n");
    while (!cpu.halted) {
        printf("PC: 0x%04X\n", cpu.PC);
        Z80Execute(&cpu);
        sleep(1);
    }

    printf("PC: 0x%04X\n", cpu.PC);

    return 0;
}
