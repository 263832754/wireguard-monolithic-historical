/* SPDX-License-Identifier: GPL-2.0
 *
 * Copyright (C) 2015-2018 Jason A. Donenfeld <Jason@zx2c4.com>. All Rights Reserved.
 */

#ifndef _WG_POLY1305_H
#define _WG_POLY1305_H

#include <linux/types.h>

enum poly1305_lengths {
	POLY1305_BLOCK_SIZE = 16,
	POLY1305_KEY_SIZE = 32,
	POLY1305_MAC_SIZE = 16
};

#if defined(CONFIG_MIPS) && defined(CONFIG_CPU_MIPS32_R2)
#define POLY1305_OPAQUE_LEN (10 * sizeof(u32))
#elif defined(CONFIG_MIPS) && defined(CONFIG_64BIT)
#define POLY1305_OPAQUE_LEN (6 * sizeof(u64))
#elif !(defined(CONFIG_X86_64) || defined(CONFIG_ARM) || defined(CONFIG_ARM64) || (defined(CONFIG_MIPS) && (defined(CONFIG_64BIT) || defined(CONFIG_CPU_MIPS32_R2))))
/* POLY1305 C version */
#define POLY1305_OPAQUE_LEN (9 * sizeof(u32))
#else
/* Default POLY1305_OPAQUE_LEN, can be removed when all lengths are known. */
#define POLY1305_OPAQUE_LEN (24 * sizeof(u64))
#endif

struct poly1305_ctx {
	u8 opaque[POLY1305_OPAQUE_LEN];
	u32 nonce[4];
	u8 data[POLY1305_BLOCK_SIZE];
	size_t num;
} __aligned(8);

void poly1305_fpu_init(void);

void poly1305_init(struct poly1305_ctx *ctx, const u8 key[POLY1305_KEY_SIZE], bool have_simd);
void poly1305_update(struct poly1305_ctx *ctx, const u8 *inp, const size_t len, bool have_simd);
void poly1305_finish(struct poly1305_ctx *ctx, u8 mac[POLY1305_MAC_SIZE], bool have_simd);

#ifdef DEBUG
bool poly1305_selftest(void);
#endif

#endif /* _WG_POLY1305_H */
