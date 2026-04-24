#include "rom.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

CartridgeHardwareBits rom_get_hw_bits(uint8_t hw_byte) {
    // clang-format off
    // Taken from https://gbdev.io/pandocs/The_Cartridge_Header.html#0147--cartridge-type
    switch (hw_byte) {
        case 0x00: return ROM_ONLY;
        case 0x01: return MBC1;
        case 0x02: return MBC1 | RAM;
        case 0x03: return MBC1 | RAM | BATTERY;
        case 0x05: return MBC2;
        case 0x06: return MBC2 | BATTERY;
        case 0x08: return ROM | RAM;
        case 0x09: return ROM | RAM | BATTERY;
        case 0x0B: return MMM01;
        case 0x0C: return MMM01 | RAM;
        case 0x0D: return MMM01 | RAM | BATTERY;
        case 0x0F: return MBC3 | TIMER | BATTERY;
        case 0x10: return MBC3 | TIMER | RAM | BATTERY;
        case 0x11: return MBC3;
        case 0x12: return MBC3 | RAM;
        case 0x13: return MBC3 | RAM | BATTERY;
        case 0x19: return MBC5;
        case 0x1A: return MBC5 | RAM;
        case 0x1B: return MBC5 | RAM | BATTERY;
        case 0x1C: return MBC5 | RUMBLE;
        case 0x1D: return MBC5 | RUMBLE | RAM;
        case 0x1E: return MBC5 | RUMBLE | RAM | BATTERY;
        case 0x20: return MBC6;
        case 0x22: return MBC7 | SENSOR | RUMBLE | RAM | BATTERY;
        case 0xFC: return POCKET_CAMERA;
        case 0xFD: return BANDAI_TAMA5;
        case 0xFE: return HuC3;
        case 0xFF: return HuC1 | RAM | BATTERY;
    }
    // clang-format on

    __builtin_unreachable();
}

const char *rom_get_hw_bits_string(uint8_t hw_byte) {
    // clang-format off
    switch (hw_byte) {
        case 0x00: return "ROM_ONLY";
        case 0x01: return "MBC1";
        case 0x02: return "MBC1 | RAM";
        case 0x03: return "MBC1 | RAM | BATTERY";
        case 0x05: return "MBC2";
        case 0x06: return "MBC2 | BATTERY";
        case 0x08: return "ROM | RAM";
        case 0x09: return "ROM | RAM | BATTERY";
        case 0x0B: return "MMM01";
        case 0x0C: return "MMM01 | RAM";
        case 0x0D: return "MMM01 | RAM | BATTERY";
        case 0x0F: return "MBC3 | TIMER | BATTERY";
        case 0x10: return "MBC3 | TIMER | RAM | BATTERY";
        case 0x11: return "MBC3";
        case 0x12: return "MBC3 | RAM";
        case 0x13: return "MBC3 | RAM | BATTERY";
        case 0x19: return "MBC5";
        case 0x1A: return "MBC5 | RAM";
        case 0x1B: return "MBC5 | RAM | BATTERY";
        case 0x1C: return "MBC5 | RUMBLE";
        case 0x1D: return "MBC5 | RUMBLE | RAM";
        case 0x1E: return "MBC5 | RUMBLE | RAM | BATTERY";
        case 0x20: return "MBC6";
        case 0x22: return "MBC7 | SENSOR | RUMBLE | RAM | BATTERY";
        case 0xFC: return "POCKET_CAMERA";
        case 0xFD: return "BANDAI_TAMA5";
        case 0xFE: return "HuC3";
        case 0xFF: return "HuC1 | RAM | BATTERY";
    }
    // clang-format on

    __builtin_unreachable();
}

Rom *rom_new(char *file) {
    int fd = open(file, O_RDONLY);
    if (fd == -1) {
        perror("open");
        return NULL;
    }

    struct stat file_stat;
    int rc = fstat(fd, &file_stat);
    if (rc != 0) {
        perror("fstat");
        return NULL;
    }

    if (file_stat.st_size < 0x180) {
        fprintf(stderr, "ROM %s is too small!\n", file);
        return NULL;
    }

    uint8_t *rom_data = mmap(0, file_stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (rom_data == MAP_FAILED) {
        perror("mmap");
        return NULL;
    }

    RomHeaderS *rom_header_s = (RomHeaderS *)(rom_data + 0x100);

    int rom_size = (32 * 1024) * (1 << rom_header_s->rom_size);
    int ram_size;

    switch (rom_header_s->ram_size) {
        case 0x00:
            ram_size = 0;
            break;
        case 0x01:
            exit(1);
            break;
        case 0x02:
            ram_size = 8 * 1024;
            break;
        case 0x03:
            ram_size = 32 * 1024;
            break;
        case 0x04:
            ram_size = 128 * 1024;
            break;
        case 0x05:
            ram_size = 64 * 1024;
            break;
    }

    RomHeader rom_header = (RomHeader){
        .rom_size = rom_size,
        // Rom is banked in 16 kib segments each
        .rom_banks = rom_size / (16 * 1024),
        .ram_size = ram_size,
        // Ram is banked in 8kib segments each
        .ram_banks = ram_size / (8 * 1024),
        .hw_bits = rom_get_hw_bits(rom_header_s->cartridge_type),
    };
    // TODO: Support later catridges with shortened titles?
    strncpy(rom_header.title, rom_header_s->title_16, 17);

    if (rom_header.rom_size != file_stat.st_size) {
        fprintf(
            stderr, "ROM %s has mismatching sizes! Reported: %d, Actual: %ld\n", file, rom_header.rom_size,
            file_stat.st_size);

        exit(1);
    }

    Rom *ret = malloc(sizeof(*ret));
    *ret = (Rom){
        .header = rom_header,
        .fd = fd,
        .data = rom_data,
    };

    printf("Loaded ROM %s\n", rom_header.title);
    printf("Loaded ROM of type %s\n", rom_get_hw_bits_string(rom_header_s->cartridge_type));
    printf("ROM Size: %d KiB, ROM Banks: %d\n", rom_header.rom_size / (1024), rom_header.rom_banks);
    printf("RAM Size: %d KiB, RAM Banks: %d\n", rom_header.ram_size / (1024), rom_header.ram_banks);

    return ret;
}

void rom_free(Rom *rom) {
    munmap(rom->data, rom->header.rom_size);
    close(rom->fd);
    free(rom->header.title);
    free(rom);
}
