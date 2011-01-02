/* 
 * 24.10.2010
 * Copyright (C) 2010 Alexander Saprykin <xelfium@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA.
 */

/* TODO: error reporting */
/* TODO: convert to hex string */

#include <string.h>
#include <stdlib.h>

#include "pmem.h"
#include "psha1.h"

struct _PHashSHA1 {
	union _buf {
		puchar		buf[64];
		puint32		buf_w[16];
	} buf;
	puint32		hash[5];

	puint32		len_high;
	puint32		len_low;
};

static puchar sha1_pad[64] = {
	0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static void sha1_swap_bytes (puint32 *data, puint words);
static void sha1_process (PHashSHA1 *ctx, const puint32 data[16]);

#define P_SHA1_ROTL(val, shift) ((val) << (shift) |  (val) >> (32 - (shift)))

#define P_SHA1_F1(x, y, z) ((x & y) | ((~x) & z))
#define P_SHA1_F2(x, y, z) (x ^ y ^ z)
#define P_SHA1_F3(x, y, z) ((x & y) | (x & z) | (y & z))

#define P_SHA1_W(W, i) 					\
(							\
	(W)[i & 0x0F] = P_SHA1_ROTL (			\
				(W)[(i - 3)  & 0x0F]	\
			      ^ (W)[(i - 8)  & 0x0F]	\
			      ^ (W)[(i - 14) & 0x0F]	\
			      ^ (W)[(i - 16) & 0x0F],	\
			      1)			\
)

#define P_SHA1_ROUND_0(a, b, c, d, e, w)		\
{							\
	e += P_SHA1_ROTL (a, 5) + P_SHA1_F1 (b, c, d)	\
	   + 0x5A827999 + w;				\
	b = P_SHA1_ROTL (b, 30);			\
}

#define P_SHA1_ROUND_1(a, b, c, d, e, w)		\
{							\
	e += P_SHA1_ROTL (a, 5) + P_SHA1_F2 (b, c, d)	\
	   + 0x6ED9EBA1 + w;				\
	b = P_SHA1_ROTL (b, 30);			\
}

#define P_SHA1_ROUND_2(a, b, c, d, e, w)		\
{							\
	e += P_SHA1_ROTL (a, 5) + P_SHA1_F3 (b, c, d)	\
	   + 0x8F1BBCDC + w;				\
	b = P_SHA1_ROTL (b, 30);			\
}

#define P_SHA1_ROUND_3(a, b, c, d, e, w)		\
{							\
	e += P_SHA1_ROTL (a, 5) + P_SHA1_F2 (b, c, d)	\
	   + 0xCA62C1D6 + w;				\
	b = P_SHA1_ROTL (b, 30);			\
}

static void
sha1_swap_bytes (puint32	*data,
		 puint		words)
{
	if (P_BYTE_ORDER == P_BIG_ENDIAN)
		return;

	while (words-- > 0) {
		*data = PUINT32_TO_BE (*data);
		++data;
	}
}

static void
sha1_process (PHashSHA1		*ctx,
	      const puint32	data[16])
{
	puint32		W[16], A, B, C, D, E;

	if (ctx == NULL)
		return;

	memcpy (W, data, 64);

	A = ctx->hash[0];
	B = ctx->hash[1];
	C = ctx->hash[2];
	D = ctx->hash[3];
	E = ctx->hash[4];

	P_SHA1_ROUND_0 (A, B, C, D, E, W[0]);
	P_SHA1_ROUND_0 (E, A, B, C, D, W[1]);
	P_SHA1_ROUND_0 (D, E, A, B, C, W[2]);
	P_SHA1_ROUND_0 (C, D, E, A, B, W[3]);
	P_SHA1_ROUND_0 (B, C, D, E, A, W[4]);
	P_SHA1_ROUND_0 (A, B, C, D, E, W[5]);
	P_SHA1_ROUND_0 (E, A, B, C, D, W[6]);
	P_SHA1_ROUND_0 (D, E, A, B, C, W[7]);
	P_SHA1_ROUND_0 (C, D, E, A, B, W[8]);
	P_SHA1_ROUND_0 (B, C, D, E, A, W[9]);
	P_SHA1_ROUND_0 (A, B, C, D, E, W[10]);
	P_SHA1_ROUND_0 (E, A, B, C, D, W[11]);
	P_SHA1_ROUND_0 (D, E, A, B, C, W[12]);
	P_SHA1_ROUND_0 (C, D, E, A, B, W[13]);
	P_SHA1_ROUND_0 (B, C, D, E, A, W[14]);
	P_SHA1_ROUND_0 (A, B, C, D, E, W[15]);
	P_SHA1_ROUND_0 (E, A, B, C, D, P_SHA1_W (W, 16));
	P_SHA1_ROUND_0 (D, E, A, B, C, P_SHA1_W (W, 17));
	P_SHA1_ROUND_0 (C, D, E, A, B, P_SHA1_W (W, 18));
	P_SHA1_ROUND_0 (B, C, D, E, A, P_SHA1_W (W, 19));

	P_SHA1_ROUND_1 (A, B, C, D, E, P_SHA1_W (W, 20));
	P_SHA1_ROUND_1 (E, A, B, C, D, P_SHA1_W (W, 21));
	P_SHA1_ROUND_1 (D, E, A, B, C, P_SHA1_W (W, 22));
	P_SHA1_ROUND_1 (C, D, E, A, B, P_SHA1_W (W, 23));
	P_SHA1_ROUND_1 (B, C, D, E, A, P_SHA1_W (W, 24));
	P_SHA1_ROUND_1 (A, B, C, D, E, P_SHA1_W (W, 25));
	P_SHA1_ROUND_1 (E, A, B, C, D, P_SHA1_W (W, 26));
	P_SHA1_ROUND_1 (D, E, A, B, C, P_SHA1_W (W, 27));
	P_SHA1_ROUND_1 (C, D, E, A, B, P_SHA1_W (W, 28));
	P_SHA1_ROUND_1 (B, C, D, E, A, P_SHA1_W (W, 29));
	P_SHA1_ROUND_1 (A, B, C, D, E, P_SHA1_W (W, 30));
	P_SHA1_ROUND_1 (E, A, B, C, D, P_SHA1_W (W, 31));
	P_SHA1_ROUND_1 (D, E, A, B, C, P_SHA1_W (W, 32));
	P_SHA1_ROUND_1 (C, D, E, A, B, P_SHA1_W (W, 33));
	P_SHA1_ROUND_1 (B, C, D, E, A, P_SHA1_W (W, 34));
	P_SHA1_ROUND_1 (A, B, C, D, E, P_SHA1_W (W, 35));
	P_SHA1_ROUND_1 (E, A, B, C, D, P_SHA1_W (W, 36));
	P_SHA1_ROUND_1 (D, E, A, B, C, P_SHA1_W (W, 37));
	P_SHA1_ROUND_1 (C, D, E, A, B, P_SHA1_W (W, 38));
	P_SHA1_ROUND_1 (B, C, D, E, A, P_SHA1_W (W, 39));

	P_SHA1_ROUND_2 (A, B, C, D, E, P_SHA1_W (W, 40));
	P_SHA1_ROUND_2 (E, A, B, C, D, P_SHA1_W (W, 41));
	P_SHA1_ROUND_2 (D, E, A, B, C, P_SHA1_W (W, 42));
	P_SHA1_ROUND_2 (C, D, E, A, B, P_SHA1_W (W, 43));
	P_SHA1_ROUND_2 (B, C, D, E, A, P_SHA1_W (W, 44));
	P_SHA1_ROUND_2 (A, B, C, D, E, P_SHA1_W (W, 45));
	P_SHA1_ROUND_2 (E, A, B, C, D, P_SHA1_W (W, 46));
	P_SHA1_ROUND_2 (D, E, A, B, C, P_SHA1_W (W, 47));
	P_SHA1_ROUND_2 (C, D, E, A, B, P_SHA1_W (W, 48));
	P_SHA1_ROUND_2 (B, C, D, E, A, P_SHA1_W (W, 49));
	P_SHA1_ROUND_2 (A, B, C, D, E, P_SHA1_W (W, 50));
	P_SHA1_ROUND_2 (E, A, B, C, D, P_SHA1_W (W, 51));
	P_SHA1_ROUND_2 (D, E, A, B, C, P_SHA1_W (W, 52));
	P_SHA1_ROUND_2 (C, D, E, A, B, P_SHA1_W (W, 53));
	P_SHA1_ROUND_2 (B, C, D, E, A, P_SHA1_W (W, 54));
	P_SHA1_ROUND_2 (A, B, C, D, E, P_SHA1_W (W, 55));
	P_SHA1_ROUND_2 (E, A, B, C, D, P_SHA1_W (W, 56));
	P_SHA1_ROUND_2 (D, E, A, B, C, P_SHA1_W (W, 57));
	P_SHA1_ROUND_2 (C, D, E, A, B, P_SHA1_W (W, 58));
	P_SHA1_ROUND_2 (B, C, D, E, A, P_SHA1_W (W, 59));

	P_SHA1_ROUND_3 (A, B, C, D, E, P_SHA1_W (W, 60));
	P_SHA1_ROUND_3 (E, A, B, C, D, P_SHA1_W (W, 61));
	P_SHA1_ROUND_3 (D, E, A, B, C, P_SHA1_W (W, 62));
	P_SHA1_ROUND_3 (C, D, E, A, B, P_SHA1_W (W, 63));
	P_SHA1_ROUND_3 (B, C, D, E, A, P_SHA1_W (W, 64));
	P_SHA1_ROUND_3 (A, B, C, D, E, P_SHA1_W (W, 65));
	P_SHA1_ROUND_3 (E, A, B, C, D, P_SHA1_W (W, 66));
	P_SHA1_ROUND_3 (D, E, A, B, C, P_SHA1_W (W, 67));
	P_SHA1_ROUND_3 (C, D, E, A, B, P_SHA1_W (W, 68));
	P_SHA1_ROUND_3 (B, C, D, E, A, P_SHA1_W (W, 69));
	P_SHA1_ROUND_3 (A, B, C, D, E, P_SHA1_W (W, 70));
	P_SHA1_ROUND_3 (E, A, B, C, D, P_SHA1_W (W, 71));
	P_SHA1_ROUND_3 (D, E, A, B, C, P_SHA1_W (W, 72));
	P_SHA1_ROUND_3 (C, D, E, A, B, P_SHA1_W (W, 73));
	P_SHA1_ROUND_3 (B, C, D, E, A, P_SHA1_W (W, 74));
	P_SHA1_ROUND_3 (A, B, C, D, E, P_SHA1_W (W, 75));
	P_SHA1_ROUND_3 (E, A, B, C, D, P_SHA1_W (W, 76));
	P_SHA1_ROUND_3 (D, E, A, B, C, P_SHA1_W (W, 77));
	P_SHA1_ROUND_3 (C, D, E, A, B, P_SHA1_W (W, 78));
	P_SHA1_ROUND_3 (B, C, D, E, A, P_SHA1_W (W, 79));

	ctx->hash[0] += A;
	ctx->hash[1] += B;
	ctx->hash[2] += C;
	ctx->hash[3] += D;
	ctx->hash[4] += E;
}

P_LIB_API void
p_sha1_reset (PHashSHA1 *ctx)
{
	if (ctx == NULL)
		return;

	memset (ctx->buf.buf, 0, 64);

	ctx->len_low = 0;
	ctx->len_high = 0;

	ctx->hash[0] = 0x67452301;
	ctx->hash[1] = 0xEFCDAB89;
	ctx->hash[2] = 0x98BADCFE;
	ctx->hash[3] = 0x10325476;
	ctx->hash[4] = 0xC3D2E1F0;
}

P_LIB_API PHashSHA1 *
p_sha1_new (void)
{
	PHashSHA1 *ret;

	if ((ret = p_malloc0 (sizeof (PHashSHA1))) == NULL)
		return NULL;

	p_sha1_reset (ret);

	return ret;
}

P_LIB_API void
p_sha1_update (PHashSHA1		*ctx,
	       const puchar		*data,
	       pint			len)
{
	puint32	left, to_fill;

	if (ctx == NULL || len <= 0)
		return;

	left = ctx->len_low & 0x3F;
	to_fill = 64 - left;

	ctx->len_low += len;
	if (ctx->len_low < (puint32) len)
		++ctx->len_high;

	if (left && (puint32) len >= to_fill) {
		memcpy (ctx->buf.buf + left, data, to_fill);
		sha1_swap_bytes (ctx->buf.buf_w, 16);
		sha1_process (ctx, ctx->buf.buf_w);

		data += to_fill;
		len -= to_fill;
		left = 0;
	}

	while (len >= 64) {
		memcpy (ctx->buf.buf, data, 64);
		sha1_swap_bytes (ctx->buf.buf_w, 16);
		sha1_process (ctx, ctx->buf.buf_w);

		data += 64;
		len -= 64;
	}

	if (len > 0)
		memcpy (ctx->buf.buf + left, data, len);
}

P_LIB_API void
p_sha1_finish (PHashSHA1	*ctx)
{
	puint32		high, low;
	pint		left, last;

	if (ctx == NULL)
		return;

	left = ctx->len_low & 0x3F;
	last = (left < 56) ? (56 - left) : (120 - left);

	low = ctx->len_low << 3;
	high = ctx->len_high << 3
	     | ctx->len_low >> 29;

	if (last > 0)
		p_sha1_update (ctx, sha1_pad, last);

	ctx->buf.buf_w[14] = high;
	ctx->buf.buf_w[15] = low;

	sha1_swap_bytes (ctx->buf.buf_w, 14);
	sha1_process (ctx, ctx->buf.buf_w);

	sha1_swap_bytes (ctx->hash, 5);
}

P_LIB_API const puchar *
p_sha1_digest (PHashSHA1 *ctx)
{
	if (ctx == NULL)
		return NULL;

	return (const puchar *) ctx->hash;
}

P_LIB_API void
p_sha1_free (PHashSHA1 *ctx)
{
	if (ctx == NULL)
		return;

	p_free (ctx);
}
