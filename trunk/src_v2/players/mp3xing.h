#ifndef __mp3xing_h
#define __mp3xing_h (1)

#include <mad.h>

struct xing {
  int flags;
  unsigned long frames;
  unsigned long bytes;
  unsigned char toc[100];
  long scale;
};

enum {
  XING_FRAMES = 0x0001,
  XING_BYTES  = 0x0002,
  XING_TOC    = 0x0004,
  XING_SCALE  = 0x0008
};

int parse_xing(struct xing *xing, struct mad_bitptr ptr, unsigned int bitlen);

#endif
