/* ----------------------------------------------------------------------- *
 *
 *   Copyright 2010 Intel Corporation; author: H. Peter Anvin
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 *   Boston MA 02110-1301, USA; either version 2 of the License, or
 *   (at your option) any later version; incorporated herein by reference.
 *
 * ----------------------------------------------------------------------- */

/*
 * memdiskfind.c
 *
 * Simple utility to search for a MEMDISK instance and output the parameters
 * needed to use the "phram" driver in Linux to map it.
 */

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include "../memdisk/mstructs.h"

#define MBFT_MIN_LENGTH	(36+4+26)

static bool valid_mbft(const struct mBFT *mbft, size_t space)
{
    uint8_t csum;
    size_t i;

    if (memcmp(mbft->acpi.signature, "mBFT", 4))
	return false;

    if (mbft->acpi.length < MBFT_MIN_LENGTH)
	return false;

    if (mbft->acpi.length > space)
	return false;

    if ((size_t)mbft->acpi.length != (size_t)mbft->mdi.bytes + 36+4)
	return false;

    csum = 0;
    for (i = 0; i < mbft->acpi.length; i++)
	csum += ((const uint8_t *)mbft)[i];

    if (csum)
	return false;

    return true;
}

static void output_params(const struct mBFT *mbft)
{
    int sector_shift = mbft->mdi.sector_shift;

    if (!sector_shift)
	sector_shift = 9;

    printf("%#x,%#x\n",
	   mbft->mdi.diskbuf, mbft->mdi.disksize << sector_shift);
}

int main(int argc, char *argv[])
{
    const char *map;
    int memfd;
    uint16_t fbm;
    const char *ptr, *end;
    size_t page = sysconf(_SC_PAGESIZE);
    size_t mapbase, maplen;
    int err = 1;

    (void)argc;

    memfd = open("/dev/mem", O_RDONLY);
    if (memfd < 0) {
	fprintf(stderr, "%s: cannot open /dev/mem: %s\n",
		argv[0], strerror(errno));
	return 2;
    }

    map = mmap(NULL, page, PROT_READ, MAP_SHARED, memfd, 0);
    if (map == MAP_FAILED) {
	fprintf(stderr, "%s: cannot map page 0: %s\n",
		argv[0], strerror(errno));
	return 2;
    }

    fbm = *(uint16_t *)(map + 0x413);

    munmap((void *)map, 4096);

    if (fbm < 64 || fbm >= 640)
	return 1;

    mapbase = (fbm << 10) & ~(page - 1);
    maplen  = 0xa0000 - mapbase;
    map = mmap(NULL, maplen, PROT_READ, MAP_SHARED, memfd, mapbase);
    if (map == MAP_FAILED) {
	fprintf(stderr, "%s: cannot map base memory: %s\n",
		argv[0], strerror(errno));
	return 2;
    }

    ptr = map + ((fbm << 10) & (page - 1));
    end = map + (0xa0000 - mapbase);
    while (ptr < end) {
	if (valid_mbft((const struct mBFT *)ptr, end-ptr)) {
	    output_params((const struct mBFT *)ptr);
	    err = 0;
	    break;
	}
	ptr += 16;
    }

    munmap((void *)map, maplen);

    return err;
}
