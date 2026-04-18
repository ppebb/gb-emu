#include "rom.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

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

    RomHeader rom_header = (RomHeader){
        .rom_size = (32 * 1024) * (1 << rom_header_s->rom_size),
    };
    // TODO: Support later catridges with shortened titles?
    strncpy(rom_header.title, rom_header_s->title_16, 17);

    if (rom_header.rom_size != file_stat.st_size) {
        fprintf(
            stderr, "ROM %s has mismatching sizes! Reported: %d, Actual: %ld\n", file, rom_header.rom_size,
            file_stat.st_size);
    }

    Rom *ret = malloc(sizeof(*ret));
    *ret = (Rom){
        .header = rom_header,
        .fd = fd,
        .data = rom_data,
    };

    return ret;
}

void rom_free(Rom *rom) {
    munmap(rom->data, rom->header.rom_size);
    close(rom->fd);
    free(rom->header.title);
    free(rom);
}
