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

static_assert(sizeof(RomHeaderS) == 0x50, "ROM header is the incorrect size");

typedef struct _RomHeader {
    char title[17];
    int rom_size;
} RomHeader;

typedef struct _Rom {
    RomHeader header;
    int fd;
    uint8_t *data;
} Rom;

Rom *rom_new(char *file);
void rom_free(Rom *rom);
