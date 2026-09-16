/* See LICENSE file for copyright and license details. */
#include "barparse.h"

#include <string.h>

int
bar_blocks(const char *src, int portrait, char *dst, int dstlen, BarBlock *out,
           int max) {
  int len = 0, nb = 0, start = 0;
  unsigned int curid = 0;
  const char *p;

  if (!src || !dst || dstlen <= 0 || !out || max <= 0)
    return 0;
  dst[0] = '\0';

  for (p = src; *p; p++) {
    unsigned char c = (unsigned char)*p;

    if (c == 0x7f) {
      /* cut marker: on portrait monitors everything before it is dropped */
      if (portrait) {
        len = 0;
        nb = 0;
        start = 0;
        curid = 0;
        dst[0] = '\0';
      }
      continue;
    }
    if (c < ' ') {
      /* control character: close the current block, tag the next one with it */
      if (len > start && nb < max) {
        out[nb].id = curid;
        out[nb].off = start;
        out[nb].end = len;
        nb++;
      }
      curid = c;
      start = len;
      continue;
    }
    if (len + 1 < dstlen)
      dst[len++] = (char)c;
  }

  if (len > start && nb < max) {
    out[nb].id = curid;
    out[nb].off = start;
    out[nb].end = len;
    nb++;
  }
  dst[len] = '\0';
  return nb;
}

/* index of the first block tagged with id, -1 when absent */
static int
block_by_id(const BarBlock *blocks, int nblocks, unsigned int id) {
  int i;

  for (i = 0; i < nblocks; i++)
    if (blocks[i].id == id)
      return i;
  return -1;
}

int
bar_pills(const char *src, const BarBlock *blocks, int nblocks, const int *ids,
          char *dst, int dstlen, BarPillCell *cells, int maxcells) {
  int i, len = 0, ncells = 0, open = 0;

  if (!dst || dstlen <= 0)
    return 0;
  dst[0] = '\0';
  if (!src || !blocks || !ids || !cells || maxcells <= 0)
    return 0;

  for (i = 0; ids[i] != -1; i++) {
    int bi, blen;

    if (ids[i] == 0) { /* pill break */
      if (open) {
        if (len + 3 < dstlen) {
          dst[len++] = '^';
          dst[len++] = ')';
          dst[len++] = '^';
          dst[len] = '\0';
        }
        if (ncells > 0)
          cells[ncells - 1].gap = 1;
        open = 0;
      }
      continue;
    }
    if ((bi = block_by_id(blocks, nblocks, (unsigned int)ids[i])) < 0)
      continue; /* the writer left this block out (empty or cut on portrait) */
    blen = blocks[bi].end - blocks[bi].off;
    if (len + blen + 4 > dstlen)
      break;
    if (!open) {
      dst[len++] = '^';
      dst[len++] = '(';
      dst[len++] = '^';
      open = 1;
    }
    memcpy(dst + len, src + blocks[bi].off, (size_t)blen);
    len += blen;
    dst[len] = '\0';
    if (ncells < maxcells) {
      cells[ncells].block = bi;
      cells[ncells].gap = 0;
      ncells++;
    }
  }

  if (open) {
    if (len + 3 < dstlen) {
      dst[len++] = '^';
      dst[len++] = ')';
      dst[len++] = '^';
      dst[len] = '\0';
    }
    if (ncells > 0)
      cells[ncells - 1].gap = 1;
  }
  return ncells;
}

int
bar_cells(int want, int n, int avail, int gap, BarCellsMode mode, int *out,
          int maxw) {
  int base, i, rem, room;

  if (n <= 0)
    return 0;
  room = avail - gap * n;
  if (room < 0)
    room = 0;

  if (want > 0 && (mode == BarCellsFixed ||
                   (mode == BarCellsFit && want * n < room))) {
    for (i = 0; i < n && i < maxw; i++)
      out[i] = want;
    return want * n + gap * n;
  }

  base = room / n;
  rem = room % n;
  for (i = 0; i < n && i < maxw; i++)
    out[i] = base + (i < rem ? 1 : 0);
  return room + gap * n;
}
