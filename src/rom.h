#pragma once

#include <stdint.h>

// See https://gbdev.io/pandocs/The_Cartridge_Header.html

typedef struct __attribute__((packed)) _RomHeaderS {
    uint8_t entry[4];
    uint8_t logo[48];
    union {
        char title_16[16];
        struct {
            char title_15[15];
            uint8_t cgb_flag;
        };
        struct {
            char title_11[11];
            char manufacturer_code[4];
            uint8_t cgb_flag_11;
        };
    };
    uint8_t new_licensee_code[2];
    uint8_t sgb_flag;
    uint8_t cartridge_type;
    uint8_t rom_size;
    uint8_t ram_size;
    uint8_t dest_code;
    uint8_t old_licensee_code;
    uint8_t version;
    uint8_t header_csum;
    uint8_t global_csum[2];
} RomHeaderS;

typedef enum _CartridgeHardwareBits {
    ROM_ONLY = 1 << 0,
    MBC1 = 1 << 1,
    MBC2 = 1 << 2,
    MBC3 = 1 << 3,
    MBC5 = 1 << 4,
    MBC6 = 1 << 5,
    MBC7 = 1 << 6,
    RAM = 1 << 7,
    ROM = 1 << 8,
    BATTERY = 1 << 9,
    RUMBLE = 1 << 10,
    MMM01 = 1 << 11,
    TIMER = 1 << 12,
    SENSOR = 1 << 13,
    HuC1 = 1 << 14,
    HuC3 = 1 << 15,
    POCKET_CAMERA = 1 << 16,
    BANDAI_TAMA5 = 1 << 17,
} CartridgeHardwareBits;

static_assert(sizeof(RomHeaderS) == 0x50, "ROM header is the incorrect size");

typedef struct _RomHeader {
    char title[17];
    int rom_size;
    int rom_banks;
    int ram_size;
    int ram_banks;
    CartridgeHardwareBits hw_bits;
} RomHeader;

typedef struct _Rom {
    RomHeader header;
    int fd;
    uint8_t *data;
} Rom;

Rom *rom_new(char *file);
void rom_info(Rom *rom);
void rom_free(Rom *rom);
