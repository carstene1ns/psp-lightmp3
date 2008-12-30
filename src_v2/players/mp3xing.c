#include <string.h>
#include <mad.h>
#include "mp3xing.h"

/* XING parsing is from the MAD winamp input plugin */

# define XING_MAGIC     (('X' << 24) | ('i' << 16) | ('n' << 8) | 'g')

int parse_xing(struct xing *xing, struct mad_bitptr ptr, unsigned int bitlen)
{
  if (bitlen < 64 || mad_bit_read(&ptr, 32) != XING_MAGIC)
    goto fail;

  xing->flags = mad_bit_read(&ptr, 32);
  bitlen -= 64;

  if (xing->flags & XING_FRAMES) {
    if (bitlen < 32)
      goto fail;

    xing->frames = mad_bit_read(&ptr, 32);
    bitlen -= 32;
  }

  if (xing->flags & XING_BYTES) {
    if (bitlen < 32)
      goto fail;

    xing->bytes = mad_bit_read(&ptr, 32);
    bitlen -= 32;
  }

  if (xing->flags & XING_TOC) {
    int i;

    if (bitlen < 800)
      goto fail;

    for (i = 0; i < 100; ++i)
      xing->toc[i] = mad_bit_read(&ptr, 8);

    bitlen -= 800;
  }

  if (xing->flags & XING_SCALE) {
    if (bitlen < 32)
      goto fail;

    xing->scale = mad_bit_read(&ptr, 32);
    bitlen -= 32;
  }

  return 1;

 fail:
  xing->flags = 0;
  return 0;
}
