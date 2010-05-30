/*
 * Copyright (C) 2010 DSLab, Lanzhou University, China
 * Author: Wu Zhangjin <wuzhangjin@gmail.com>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
	struct stat sb;
	unsigned long long vmlinux_size, vmlinux_load_addr, vmlinuz_load_addr;

	if (argc != 3) {
		fprintf(stderr, "Usage: %s <pathname> <vmlinux_load_addr>\n",
				argv[0]);
		return -1;
	}

	if (stat(argv[1], &sb) == -1) {
		perror("stat");
		return -1;
	}

	/* Convert hex characters to dec number */
	sscanf(argv[2], "%llx", &vmlinux_load_addr);

	vmlinux_size = (unsigned long long)sb.st_size;
	vmlinuz_load_addr = vmlinux_load_addr + vmlinux_size;

	/* Align with 65536 */
	vmlinuz_load_addr += (65536 - vmlinux_size % 65536);

	printf("0x%llx\n", vmlinuz_load_addr);

	return 0;
}
