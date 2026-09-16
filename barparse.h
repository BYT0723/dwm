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
   its value. 0x7f is dropped too: it used to be the portrait cut marker, and
   the orientation is picked from config now. Blocks without visible text are
   not emitted. Records at most max blocks in out and returns the count. */
int bar_blocks(const char *src, char *dst, int dstlen, BarBlock *out, int max);

/* Build the drawn pill string for the status block ids. src/blocks are the
   output of bar_blocks. ids lists block ids in draw order; 0 starts a new
   pill and -1 ends the list. Each pill is wrapped in the ^( .. ^) markers the
   renderer turns into rounded caps, and ids with no matching block are
   skipped, so a pill whose blocks are all absent emits nothing. Records at
   most maxcells drawn blocks in cells and returns their count. */
int bar_pills(const char *src, const BarBlock *blocks, int nblocks,
              const int *ids, char *dst, int dstlen, BarPillCell *cells,
              int maxcells);

/* How a row of cells treats the room it is given. */
typedef enum {
  BarCellsFit,   /* nominal width when it fits, otherwise levelled to avail */
  BarCellsFill,  /* always levelled to fill avail exactly */
  BarCellsFixed, /* always the nominal width, even when it overflows avail */
} BarCellsMode;

/* Lay out n equal-width cells inside avail, one gap after each. The nominal
   width is `want`; BarCellsFit keeps it while the cells fit, BarCellsFill
   always levels the row to avail, BarCellsFixed never levels (the row may
   then be wider than avail, and the caller drops what does not fit). Leveling
   gives the first avail % n cells the extra pixel. Writes at most maxw widths
   (out may be NULL) and returns the row's total width, gaps included. */
int bar_cells(int want, int n, int avail, int gap, BarCellsMode mode, int *out,
              int maxw);

/* Where the center zone starts inside the bar window. barw is that window's
   inner width (side padding and the systray already taken out), stw the
   systray width reserved inside the monitor, leftw/rightw the widths of the
   side zones and centerw the middle zone's own width. The zone is centred on
   the monitor: the bar window is narrower than the monitor by the systray, so
   centring on the window would sit half a systray left of the monitor's
   middle. It is then pushed back inside the span the side zones leave free,
   so the caller must pass a centerw that fits it (centerw <= barw - leftw -
   rightw); such a middle can only fill that span. */
int bar_centerx(int barw, int stw, int leftw, int rightw, int centerw);

#endif
