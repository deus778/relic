/*
 * RELIC is an Efficient LIbrary for Cryptography
 * Copyright (C) 2007-2013 RELIC Authors
 *
 * This file is part of RELIC. RELIC is legal property of its developers,
 * whose names are not listed here. Please refer to the COPYRIGHT file
 * for contact information.
 *
 * RELIC is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * RELIC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with RELIC. If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file
 *
 * Implementation of utilities for pseudo-random number generation.
 *
 * @version $Id$
 * @ingroup rand
 */

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "relic_conf.h"
#include "relic_core.h"
#include "relic_label.h"
#include "relic_rand.h"
#include "relic_md.h"
#include "relic_err.h"

#if RAND == UDEV || SEED == DEV || SEED == UDEV

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#elif SEED == WCGR

/* Avoid redefinition warning. */
#undef ERROR
#undef WORD
#undef DOUBLE

#include <windows.h>
#include <Wincrypt.h>

#endif

/*============================================================================*/
/* Private definitions                                                        */
/*============================================================================*/

/**
 * The path to the char device that supplies entropy.
 */
#if SEED == DEV
#define RAND_PATH		"/dev/random"
#else
#define RAND_PATH		"/dev/urandom"
#endif

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

void rand_init() {
	unsigned char buf[SEED_SIZE];

#if RAND == UDEV
	int *fd = (int *)&(core_get()->rand);

	*fd = open(RAND_PATH, O_RDONLY);
	if (*fd == -1) {
		THROW(ERR_NO_FILE);
	}
#else

#if SEED == ZERO

	memset(buf, 0, SEED_SIZE);

#elif SEED == DEV || SEED == UDEV
	int fd, c, l;

	fd = open(RAND_PATH, O_RDONLY);
	if (fd == -1) {
		THROW(ERR_NO_FILE);
	}

	l = 0;
	do {
		c = read(fd, buf + l, SEED_SIZE - l);
		l += c;
		if (c == -1) {
			THROW(ERR_NO_READ);
		}
	} while (l < SEED_SIZE);

	if (fd != -1) {
		close(fd);
	}
#elif SEED == LIBC

#if OPSYS == FREEBSD
	srandom(1);
	for (int i = 0; i < SEED_SIZE; i++) {
		buf[i] = (unsigned char)random();
	}
#else
	srand(1);
	for (int i = 0; i < SEED_SIZE; i++) {
		buf[i] = (unsigned char)rand();
	}
#endif

#elif SEED == WCGR
	HCRYPTPROV hCryptProv;

	if (!CryptAcquireContext(&hCryptProv, NULL, NULL, PROV_RSA_FULL,
					CRYPT_VERIFYCONTEXT | CRYPT_SILENT)) {
		THROW(ERR_NO_FILE);
	}
	if (hCryptProv && !CryptGenRandom(hCryptProv, SEED_SIZE, buf)) {
		THROW(ERR_NO_READ);
	}
	if (hCryptProv && !CryptReleaseContext(hCryptProv, 0)) {
		THROW(ERR_NO_READ);
	}
#endif

#endif /* RAND == UDEV */

	core_get()->seeded = 0;
	rand_seed(buf, SEED_SIZE);
}

void rand_clean() {

#if RAND == UDEV
	int *fd = (int *)&(core_get()->rand);
	close(*fd);
#endif

	memset(core_get()->rand, 0, sizeof(core_get()->rand));
	core_get()->seeded = 0;
}
