/* See LICENSE file for copyright and license details. */
#include "barparse.h"

#include <string.h>

/* record the pending [start, len) slice as a block tagged with curid */
static int emit_block(BarBlock *out, int max, int nb, unsigned int curid,
                      int start, int len) {
  if (len > start && nb < max) {
    out[nb].id = curid;
    out[nb].off = start;
    out[nb].end = len;
    return nb + 1;
  }
  return nb;
}

int bar_blocks(const char *src, char *dst, int dstlen, BarBlock *out, int max) {
  int len = 0, nb = 0, start = 0;
  unsigned int curid = 0;
  const char *p;

  if (!src || !dst || dstlen <= 0 || !out || max <= 0)
    return 0;
  dst[0] = '\0';

  for (p = src; *p; p++) {
    unsigned char c = (unsigned char)*p;

    if (c == 0x7f)
      continue; /* the portrait cut marker is gone; ignore a stale one */
    if (c < ' ') {
      /* control character: close the current block, tag the next one with it */
      nb = emit_block(out, max, nb, curid, start, len);
      curid = c;
      start = len;
      continue;
    }
    if (len + 1 < dstlen)
      dst[len++] = (char)c;
  }

  nb = emit_block(out, max, nb, curid, start, len);
  dst[len] = '\0';
  return nb;
}

/* index of the first block tagged with id, -1 when absent */
static int block_by_id(const BarBlock *blocks, int nblocks, unsigned int id) {
  int i;

  for (i = 0; i < nblocks; i++)
    if (blocks[i].id == id)
      return i;
  return -1;
}

/* append the ^PILL_CLOSE pill terminator and mark the gap after the last cell */
static int close_pill(char *dst, int dstlen, int len, BarPillCell *cells,
                      int ncells) {
  if (len + 3 < dstlen) {
    dst[len++] = '^';
    dst[len++] = PILL_CLOSE;
    dst[len++] = '^';
    dst[len] = '\0';
  }
  if (ncells > 0)
    cells[ncells - 1].gap = 1;
  return len;
}

int bar_pills(const char *src, const BarBlock *blocks, int nblocks,
              const int *ids, char *dst, int dstlen, BarPillCell *cells,
              int maxcells) {
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
        len = close_pill(dst, dstlen, len, cells, ncells);
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
      dst[len++] = PILL_OPEN;
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

  if (open)
    len = close_pill(dst, dstlen, len, cells, ncells);
  return ncells;
}

int bar_cells(int want, int n, int avail, int gap, BarCellsMode mode, int *out,
              int maxw) {
  int base, i, rem, room;

  if (n <= 0)
    return 0;
  room = avail - gap * n;
  if (room < 0)
    room = 0;

  if (want > 0 &&
      (mode == BarCellsFixed || (mode == BarCellsFit && want * n < room))) {
    if (out)
      for (i = 0; i < n && i < maxw; i++)
        out[i] = want;
    return want * n + gap * n;
  }

  base = room / n;
  rem = room % n;
  if (out)
    for (i = 0; i < n && i < maxw; i++)
      out[i] = base + (i < rem ? 1 : 0);
  return room + gap * n;
}

int bar_centerx(int barw, int stw, int leftw, int rightw, int centerw) {
  /* the monitor's middle in bar-window coordinates; the window is stw short
     of the monitor, so the systray width comes back into the base */
  int x = (barw + stw - centerw) / 2;

  if (x < leftw)
    x = leftw;
  if (x + centerw > barw - rightw)
    x = barw - rightw - centerw;
  return x;
}

int bar_tabdotroom(int dot) { return dot > 0 ? 2 * dot + 1 : 0; }

int bar_tabiconsize(int iconsize, int dot) {
  int room = bar_tabdotroom(dot), size;

  if (room <= 0)
    return iconsize;
  size = iconsize - room - 1; /* one spare pixel keeps the dot off the icon */
  return size < 8 ? 8 : size;
}
