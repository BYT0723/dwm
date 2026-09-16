/* See LICENSE file for copyright and license details.
 *
 * Status block splitter: dwm's status text (root WM_NAME) carries its logical
 * blocks as control characters. This module turns it into a flat visible
 * string plus a list of blocks, so drawing and click resolution share one
 * parse. Kept free of X dependencies on purpose, so it can be unit tested. */
#ifndef BARPARSER_H
#define BARPARSER_H

/* One status block: the visible slice [off, end) inside the filtered buffer,
   tagged with the control character that introduced it. The run before any
   control character has id 0. Offsets index the filtered buffer, not src. */
typedef struct {
  unsigned int id;
  int off, end;
} BarBlock;

/* One drawn block of a pill: which parsed block it came from, and whether a
   pill gap follows it (true for the last block of each pill). */
typedef struct {
  int block;
  int gap;
} BarPillCell;

/* Filter src into dst (NUL-terminated, at most dstlen bytes), dropping
   control characters. A character below 0x20 starts a new block tagged with
   its value. 0x7f is not a block marker: when portrait is set it restarts the
   output (dropping everything before it), otherwise it is skipped. Blocks
   without visible text are not emitted. Records at most max blocks in out and
   returns the block count. */
int bar_blocks(const char *src, int portrait, char *dst, int dstlen,
               BarBlock *out, int max);

/* Build the drawn pill string for the status block ids. src/blocks are the
   output of bar_blocks. ids lists block ids in draw order; 0 starts a new
   pill and -1 ends the list. Each pill is wrapped in the ^( .. ^) markers the
   renderer turns into rounded caps, and ids with no matching block are
   skipped, so a pill whose blocks are all absent emits nothing. Records at
   most maxcells drawn blocks in cells and returns their count. */
int bar_pills(const char *src, const BarBlock *blocks, int nblocks,
              const int *ids, char *dst, int dstlen, BarPillCell *cells,
              int maxcells);

#endif
