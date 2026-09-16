/* See LICENSE file for copyright and license details.
 *
 * Unit tests for barparse.c: the status block splitter is the only piece of
 * this feature with real branching (control-character ids, portrait 0x7f
 * reset, ^..^ codes, empty blocks), so it is kept free of X dependencies and
 * tested standalone. Build and run with `make test`. */
#include "../barparse.h"

#include <stdio.h>
#include <string.h>

static int failures = 0;
static int checks = 0;

#define CHECK(cond, ...)                                                       \
  do {                                                                         \
    checks++;                                                                  \
    if (!(cond)) {                                                             \
      failures++;                                                              \
      printf("FAIL %s:%d: ", __FILE__, __LINE__);                              \
      printf(__VA_ARGS__);                                                     \
      printf("\n");                                                            \
    }                                                                          \
  } while (0)

/* compare the block list against an expected id/off/end triple list */static void
expect(const char *name, const char *src, int portrait, int dstlen, int max,       const BarBlock *want, int nwant, const char *wantdst) {
  char dst[256];
  BarBlock got[32];
  int n, i;

  memset(got, 0, sizeof(got));
  n = bar_blocks(src, portrait, dst, dstlen > 0 ? dstlen : (int)sizeof(dst),
                 got, max > 0 ? max : (int)(sizeof(got) / sizeof(got[0])));

  CHECK(n == nwant, "%s: block count %d, want %d", name, n, nwant);
  for (i = 0; i < n && i < nwant; i++)
    CHECK(got[i].id == want[i].id && got[i].off == want[i].off &&
              got[i].end == want[i].end,
          "%s: block %d = {id=%u,off=%d,end=%d}, want {id=%u,off=%d,end=%d}",
          name, i, got[i].id, got[i].off, got[i].end, want[i].id, want[i].off,
          want[i].end);
  if (wantdst)
    CHECK(strcmp(dst, wantdst) == 0, "%s: dst '%s', want '%s'", name, dst,
          wantdst);
}

/* build pills for src with the given id list and compare the result */
static void
pills_case(const char *name, const char *src, const int *ids,
           const char *wantdst, int nwant, const int *wantblock,
           const int *wantgap) {
  char sbuf[256], obuf[256];
  BarBlock blocks[32];
  BarPillCell cells[32];
  int nblocks, n, i;

  nblocks = bar_blocks(src, 0, sbuf, sizeof sbuf, blocks,
                       (int)(sizeof(blocks) / sizeof(blocks[0])));
  n = bar_pills(sbuf, blocks, nblocks, ids, obuf, sizeof obuf, cells,
                (int)(sizeof(cells) / sizeof(cells[0])));

  CHECK(n == nwant, "%s: cell count %d, want %d", name, n, nwant);
  CHECK(strcmp(obuf, wantdst) == 0, "%s: dst '%s', want '%s'", name, obuf,
        wantdst);
  for (i = 0; i < n && i < nwant; i++) {
    CHECK(cells[i].block == wantblock[i], "%s: cell %d block %d, want %d",
          name, i, cells[i].block, wantblock[i]);
    CHECK(cells[i].gap == wantgap[i], "%s: cell %d gap %d, want %d", name, i,
          cells[i].gap, wantgap[i]);
  }
}

static void
pills(void) {
  { /* two pills, each wrapped in cap markers, gap on each last cell */
    const int ids[] = {1, 0, 2, -1};
    const int wb[] = {0, 1}, wg[] = {1, 1};
    pills_case("pills-basic", "\x01""a\x02""b", ids, "^(^a^)^^(^b^)^", 2, wb, wg);
  }
  { /* ids within a pill are drawn in id-list order and share the caps */
    const int ids[] = {2, 1, -1};
    const int wb[] = {1, 0}, wg[] = {0, 1};
    pills_case("pills-order", "\x01""a\x02""b", ids, "^(^ba^)^", 2, wb, wg);
  }
  { /* absent ids are skipped; a pill of only absent ids emits nothing */
    const int ids[] = {9, 1, 0, 9, -1};
    const int wb[] = {0}, wg[] = {1};
    pills_case("pills-missing", "\x01""a", ids, "^(^a^)^", 1, wb, wg);
  }
  { /* a trailing pill break still closes the open pill */
    const int ids[] = {1, 0, -1};
    const int wb[] = {0}, wg[] = {1};
    pills_case("pills-trailing", "\x01""a", ids, "^(^a^)^", 1, wb, wg);
  }
}

/* tab row cell layout: n equal-width cells inside avail, gap after each */
static void
cells_case(const char *name, BarCellsMode mode, int want, int n, int avail,
           int gap, int wanttotal, const int *wantw, int maxw) {
  int got[8];
  int total, i;

  for (i = 0; i < 8; i++)
    got[i] = -1;
  total = bar_cells(want, n, avail, gap, mode, got, maxw);

  CHECK(total == wanttotal, "%s: total %d, want %d", name, total, wanttotal);
  for (i = 0; i < n && i < maxw && i < 8; i++)
    CHECK(got[i] == wantw[i], "%s: cell %d width %d, want %d", name, i, got[i],
          wantw[i]);
  for (; i < 8; i++)
    CHECK(got[i] == -1, "%s: cell %d written (%d) past maxw %d", name, i, got[i],
          maxw);
}

static void
cells(void) {
  { /* no cells */
    const int w[] = {0};
    cells_case("cells-none", BarCellsFit, 64, 0, 200, 4, 0, w, 8);
  }
  { /* nominal width fits: keep it, the row is not stretched */
    const int w[] = {64, 64};
    cells_case("cells-fit", BarCellsFit, 64, 2, 200, 4, 136, w, 8);
  }
  { /* exactly fits: same widths either branch takes */
    const int w[] = {64, 64};
    cells_case("cells-exact", BarCellsFit, 64, 2, 136, 4, 136, w, 8);
  }
  { /* does not fit: levelled to fill avail, remainder to the first cells */
    const int w[] = {30, 29, 29};
    cells_case("cells-stretch", BarCellsFit, 64, 3, 100, 4, 100, w, 8);
  }
  { /* less room than the gaps alone */
    const int w[] = {0, 0};
    cells_case("cells-nogap", BarCellsFit, 64, 2, 5, 4, 8, w, 8);
  }
  { /* non-positive nominal width falls back to stretching */
    const int w[] = {46, 46};
    cells_case("cells-zero", BarCellsFit, 0, 2, 100, 4, 100, w, 8);
  }
  { /* one cell */
    const int w[] = {10};
    cells_case("cells-one", BarCellsFit, 10, 1, 100, 4, 14, w, 8);
  }
  { /* maxw clamps the writes, not the total */
    const int w[] = {64, 64};
    cells_case("cells-maxw", BarCellsFit, 64, 3, 400, 4, 204, w, 2);
  }
  { /* fill: levelled even though the nominal width fits */
    const int w[] = {96, 96};
    cells_case("cells-fill", BarCellsFill, 64, 2, 200, 4, 200, w, 8);
  }
  { /* fill ties the total to avail exactly */
    const int w[] = {30, 29, 29};
    cells_case("cells-fill-rem", BarCellsFill, 64, 3, 100, 4, 100, w, 8);
  }
  { /* fixed: kept even when it overflows avail */
    const int w[] = {64, 64, 64};
    cells_case("cells-fixed", BarCellsFixed, 64, 3, 100, 4, 204, w, 8);
  }
  { /* fixed with no nominal width falls back to levelling */
    const int w[] = {30, 29, 29};
    cells_case("cells-fixed-zero", BarCellsFixed, 0, 3, 100, 4, 100, w, 8);
  }
}

int
main(void) {
  { /* plain text: single leading block with id 0 */
    const BarBlock w[] = {{0, 0, 3}};
    expect("plain", "abc", 0, 0, 0, w, 1, "abc");
  }

  { /* control characters delimit blocks and keep their value as id */
    const BarBlock w[] = {{1, 0, 3}, {2, 3, 5}};
    expect("split", "\x01""abc\x02""de", 0, 0, 0, w, 2, "abcde");
  }

  { /* an empty block (control char with no text) is not emitted */
    const BarBlock w[] = {{0x0c, 0, 4}};
    expect("empty", "\x0d\x0c""mail", 0, 0, 0, w, 1, "mail");
  }

  { /* ^..^ status2d codes are ordinary text: they never split a block */
    const BarBlock w[] = {{0, 0, 7}, {1, 7, 8}};
    expect("codes", "^b#000^\x01""x", 0, 0, 0, w, 2, "^b#000^x");
  }

  { /* landscape: 0x7f is skipped, both halves stay in one block */
    const BarBlock w[] = {{0, 0, 2}};
    expect("landscape-7f", "a\x7f" "b", 0, 0, 0, w, 1, "ab");
  }

  { /* portrait: 0x7f drops everything before it */
    const BarBlock w[] = {{0, 0, 1}};
    expect("portrait-7f", "a\x7f" "b", 1, 0, 0, w, 1, "b");
  }

  { /* portrait reset also drops already recorded blocks */
    const BarBlock w[] = {{2, 0, 1}};
    expect("portrait-reset", "\x01""a\x7f\x02""b", 1, 0, 0, w, 1, "b");
  }

  { /* dst truncation keeps the blocks that fit */
    const BarBlock w[] = {{0, 0, 3}};
    expect("truncate", "abcdef", 0, 4, 0, w, 1, "abc");
  }

  { /* max bounds the recorded blocks */
    const BarBlock w[] = {{1, 0, 1}};
    expect("maxblocks", "\x01""a\x02""b", 0, 0, 1, w, 1, "ab");
  }

  { /* empty input */
    expect("empty-input", "", 0, 0, 0, NULL, 0, "");
  }

  pills();
  cells();

  printf("%s: %d checks, %d failures\n", failures ? "FAILED" : "ok", checks,
         failures);
  return failures ? 1 : 0;
}
