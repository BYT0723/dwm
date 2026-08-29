/* See LICENSE file for copyright and license details. */
#include <Imlib2.h>
#include <X11/Xft/Xft.h>
#include <X11/Xlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "drw.h"
#include "util.h"

#define UTF_INVALID 0xFFFD
#define UTF_SIZ 4

#define AA_SAMPLES 8 /* subsamples per pixel for rounded-corner coverage */

static const unsigned char utfbyte[UTF_SIZ + 1] = {0x80, 0, 0xC0, 0xE0, 0xF0};
static const unsigned char utfmask[UTF_SIZ + 1] = {0xC0, 0x80, 0xE0, 0xF0,
                                                   0xF8};
static const long utfmin[UTF_SIZ + 1] = {0, 0, 0x80, 0x800, 0x10000};
static const long utfmax[UTF_SIZ + 1] = {0x10FFFF, 0x7F, 0x7FF, 0xFFFF,
                                         0x10FFFF};

static long utf8decodebyte(const char c, size_t *i) {
  for (*i = 0; *i < (UTF_SIZ + 1); ++(*i))
    if (((unsigned char)c & utfmask[*i]) == utfbyte[*i])
      return (unsigned char)c & ~utfmask[*i];
  return 0;
}

static size_t utf8validate(long *u, size_t i) {
  if (!BETWEEN(*u, utfmin[i], utfmax[i]) || BETWEEN(*u, 0xD800, 0xDFFF))
    *u = UTF_INVALID;
  for (i = 1; *u > utfmax[i]; ++i)
    ;
  return i;
}

static size_t utf8decode(const char *c, long *u, size_t clen) {
  size_t i, j, len, type;
  long udecoded;

  *u = UTF_INVALID;
  if (!clen)
    return 0;
  udecoded = utf8decodebyte(c[0], &len);
  if (!BETWEEN(len, 1, UTF_SIZ))
    return 1;
  for (i = 1, j = 1; i < clen && j < len; ++i, ++j) {
    udecoded = (udecoded << 6) | utf8decodebyte(c[i], &type);
    if (type)
      return j;
  }
  if (j < len)
    return 0;
  *u = udecoded;
  utf8validate(u, len);

  return len;
}

Drw *drw_create(Display *dpy, int screen, Window root, unsigned int w,
                unsigned int h, Visual *visual, unsigned int depth,
                Colormap cmap) {
  Drw *drw = ecalloc(1, sizeof(Drw));

  drw->dpy = dpy;
  drw->screen = screen;
  drw->root = root;
  drw->w = w;
  drw->h = h;
  drw->visual = visual;
  drw->depth = depth;
  drw->cmap = cmap;
  drw->drawable = XCreatePixmap(dpy, root, w, h, depth);
  drw->picture = XRenderCreatePicture(
      dpy, drw->drawable, XRenderFindVisualFormat(dpy, visual), 0, NULL);
  drw->gc = XCreateGC(dpy, drw->drawable, 0, NULL);
  XSetLineAttributes(dpy, drw->gc, 1, LineSolid, CapButt, JoinMiter);

  return drw;
}

void drw_resize(Drw *drw, unsigned int w, unsigned int h) {
  if (!drw)
    return;

  drw->w = w;
  drw->h = h;
  if (drw->picture)
    XRenderFreePicture(drw->dpy, drw->picture);
  if (drw->drawable)
    XFreePixmap(drw->dpy, drw->drawable);
  drw->drawable = XCreatePixmap(drw->dpy, drw->root, w, h, drw->depth);
  drw->picture = XRenderCreatePicture(
      drw->dpy, drw->drawable, XRenderFindVisualFormat(drw->dpy, drw->visual),
      0, NULL);
}

void drw_free(Drw *drw) {
  XRenderFreePicture(drw->dpy, drw->picture);
  XFreePixmap(drw->dpy, drw->drawable);
  XFreeGC(drw->dpy, drw->gc);
  drw_fontset_free(drw->fonts);
  free(drw);
}

/* This function is an implementation detail. Library users should use
 * drw_fontset_create instead.
 */
static Fnt *xfont_create(Drw *drw, const char *fontname,
                         FcPattern *fontpattern) {
  Fnt *font;
  XftFont *xfont = NULL;
  FcPattern *pattern = NULL;

  if (fontname) {
    /* Using the pattern found at font->xfont->pattern does not yield the
     * same substitution results as using the pattern returned by
     * FcNameParse; using the latter results in the desired fallback
     * behaviour whereas the former just results in missing-character
     * rectangles being drawn, at least with some fonts. */
    if (!(xfont = XftFontOpenName(drw->dpy, drw->screen, fontname))) {
      fprintf(stderr, "error, cannot load font from name: '%s'\n", fontname);
      return NULL;
    }
    if (!(pattern = FcNameParse((FcChar8 *)fontname))) {
      fprintf(stderr, "error, cannot parse font name to pattern: '%s'\n",
              fontname);
      XftFontClose(drw->dpy, xfont);
      return NULL;
    }
  } else if (fontpattern) {
    /* XftFontOpenPattern takes ownership of fontpattern; keep a copy so
     * drw_text's fallback can reuse drw->fonts->pattern */
    pattern = FcPatternDuplicate(fontpattern);
    if (!pattern || !(xfont = XftFontOpenPattern(drw->dpy, fontpattern))) {
      fprintf(stderr, "error, cannot load font from pattern.\n");
      if (pattern)
        FcPatternDestroy(pattern);
      FcPatternDestroy(fontpattern); /* not consumed on failure */
      return NULL;
    }
  } else {
    die("no font specified.");
  }

  /* Do not allow using color fonts. This is a workaround for a BadLength
   * error from Xft with color glyphs. Modelled on the Xterm workaround. See
   * https://bugzilla.redhat.com/show_bug.cgi?id=1498269
   * https://lists.suckless.org/dev/1701/30932.html
   * https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=916349
   * and lots more all over the internet.
   */
  FcBool iscol;
  if (FcPatternGetBool(xfont->pattern, FC_COLOR, 0, &iscol) == FcResultMatch &&
      iscol) {
    XftFontClose(drw->dpy, xfont);
    if (pattern)
      FcPatternDestroy(pattern);
    return NULL;
  }

  font = ecalloc(1, sizeof(Fnt));
  font->xfont = xfont;
  font->pattern = pattern;
  font->h = xfont->ascent + xfont->descent;
  font->dpy = drw->dpy;

  return font;
}

static void xfont_free(Fnt *font) {
  if (!font)
    return;
  if (font->pattern)
    FcPatternDestroy(font->pattern);
  XftFontClose(font->dpy, font->xfont);
  free(font);
}

Fnt *drw_fontset_create(Drw *drw, const char *fonts[], size_t fontcount) {
  Fnt *cur, *ret = NULL;
  size_t i;

  if (!drw || !fonts)
    return NULL;

  for (i = 1; i <= fontcount; i++) {
    if ((cur = xfont_create(drw, fonts[fontcount - i], NULL))) {
      cur->next = ret;
      ret = cur;
    }
  }
  return (drw->fonts = ret);
}

void drw_fontset_free(Fnt *font) {
  if (font) {
    drw_fontset_free(font->next);
    xfont_free(font);
  }
}

#define CLR_CACHE_SZ 64

void drw_clr_create(Drw *drw, Clr *dest, const char *clrname,
                    unsigned int alpha) {
  static Clr cache[CLR_CACHE_SZ];
  static unsigned int cachealpha[CLR_CACHE_SZ];
  static char cachename[CLR_CACHE_SZ][8];
  static int cacheidx = 0;
  int i;

  if (!drw || !dest || !clrname)
    return;

  for (i = 0; i < cacheidx; i++)
    if (cachealpha[i] == alpha && !strcmp(cachename[i], clrname)) {
      *dest = cache[i];
      return;
    }

  if (!XftColorAllocName(drw->dpy, drw->visual, drw->cmap, clrname, dest))
    die("error, cannot allocate color '%s'", clrname);

  dest->pixel = (dest->pixel & 0x00ffffffU) | (alpha << 24);

  if (cacheidx < CLR_CACHE_SZ && strlen(clrname) < 8) {
    strcpy(cachename[cacheidx], clrname);
    cachealpha[cacheidx] = alpha;
    cache[cacheidx] = *dest;
    cacheidx++;
  }
}

/* Wrapper to create color schemes. The caller has to call free(3) on the
 * returned color scheme when done using it. */
Clr *drw_scm_create(Drw *drw, char *clrnames[], const unsigned int alphas[],
                    size_t clrcount) {
  size_t i;
  Clr *ret;

  /* need at least two colors for a scheme */
  if (!drw || !clrnames || clrcount < 2 ||
      !(ret = ecalloc(clrcount, sizeof(XftColor))))
    return NULL;

  for (i = 0; i < clrcount; i++)
    drw_clr_create(drw, &ret[i], clrnames[i], alphas[i]);
  return ret;
}

void drw_setfontset(Drw *drw, Fnt *set) {
  if (drw)
    drw->fonts = set;
}

void drw_setscheme(Drw *drw, Clr *scm) {
  if (drw)
    drw->scheme = scm;
}

Picture drw_picture_create_resized(Drw *drw, char *src, unsigned int srcw,
                                   unsigned int srch, unsigned int dstw,
                                   unsigned int dsth) {
  Pixmap pm;
  Picture pic;
  GC gc;

  if (srcw <= (dstw << 1u) && srch <= (dsth << 1u)) {
    XImage img = {srcw,
                  srch,
                  0,
                  ZPixmap,
                  src,
                  ImageByteOrder(drw->dpy),
                  BitmapUnit(drw->dpy),
                  BitmapBitOrder(drw->dpy),
                  32,
                  32,
                  0,
                  32,
                  0,
                  0,
                  0};
    XInitImage(&img);

    pm = XCreatePixmap(drw->dpy, drw->root, srcw, srch, 32);
    gc = XCreateGC(drw->dpy, pm, 0, NULL);
    XPutImage(drw->dpy, pm, gc, &img, 0, 0, 0, 0, srcw, srch);
    XFreeGC(drw->dpy, gc);

    pic = XRenderCreatePicture(
        drw->dpy, pm, XRenderFindStandardFormat(drw->dpy, PictStandardARGB32),
        0, NULL);
    XFreePixmap(drw->dpy, pm);

    XRenderSetPictureFilter(drw->dpy, pic, FilterBilinear, NULL, 0);
    XTransform xf;
    xf.matrix[0][0] = (srcw << 16u) / dstw;
    xf.matrix[0][1] = 0;
    xf.matrix[0][2] = 0;
    xf.matrix[1][0] = 0;
    xf.matrix[1][1] = (srch << 16u) / dsth;
    xf.matrix[1][2] = 0;
    xf.matrix[2][0] = 0;
    xf.matrix[2][1] = 0;
    xf.matrix[2][2] = 65536;
    XRenderSetPictureTransform(drw->dpy, pic, &xf);
  } else {
    Imlib_Image origin =
        imlib_create_image_using_data(srcw, srch, (DATA32 *)src);
    if (!origin)
      return None;
    imlib_context_set_image(origin);
    imlib_image_set_has_alpha(1);
    Imlib_Image scaled =
        imlib_create_cropped_scaled_image(0, 0, srcw, srch, dstw, dsth);
    imlib_free_image_and_decache();
    if (!scaled)
      return None;
    imlib_context_set_image(scaled);
    imlib_image_set_has_alpha(1);

    XImage img = {dstw,
                  dsth,
                  0,
                  ZPixmap,
                  (char *)imlib_image_get_data_for_reading_only(),
                  ImageByteOrder(drw->dpy),
                  BitmapUnit(drw->dpy),
                  BitmapBitOrder(drw->dpy),
                  32,
                  32,
                  0,
                  32,
                  0,
                  0,
                  0};
    XInitImage(&img);

    pm = XCreatePixmap(drw->dpy, drw->root, dstw, dsth, 32);
    gc = XCreateGC(drw->dpy, pm, 0, NULL);
    XPutImage(drw->dpy, pm, gc, &img, 0, 0, 0, 0, dstw, dsth);
    imlib_free_image_and_decache();
    XFreeGC(drw->dpy, gc);

    pic = XRenderCreatePicture(
        drw->dpy, pm, XRenderFindStandardFormat(drw->dpy, PictStandardARGB32),
        0, NULL);
    XFreePixmap(drw->dpy, pm);
  }

  return pic;
}

void drw_rect(Drw *drw, int x, int y, unsigned int w, unsigned int h,
              int filled, int invert) {
  if (!drw || !drw->scheme)
    return;
  XSetForeground(drw->dpy, drw->gc,
                 invert ? drw->scheme[ColBg].pixel : drw->scheme[ColFg].pixel);
  if (filled)
    XFillRectangle(drw->dpy, drw->drawable, drw->gc, x, y, w, h);
  else
    XDrawRectangle(drw->dpy, drw->drawable, drw->gc, x, y, w - 1, h - 1);
}

/* border RGB with alpha forced opaque: outlines must stay visible even
 * when the scheme's border alpha is TRANSPARENT */
static unsigned long drw_border_pixel(Drw *drw) {
  return (drw->scheme[ColBorder].pixel & 0x00ffffffU) | 0xff000000U;
}

/* expand a 32-bit aarrggbb pixel into a 16-bit-per-channel XRenderColor */
static XRenderColor drw_xrender_color(unsigned long p) {
  return (XRenderColor){.red = ((p >> 16) & 0xff) * 0x101,
                        .green = ((p >> 8) & 0xff) * 0x101,
                        .blue = (p & 0xff) * 0x101,
                        .alpha = ((p >> 24) & 0xff) * 0x101};
}

/* filled rectangle in the border color, painted via XRender so the outline
 * shares the rounded caps' alpha behavior on the ARGB drawable */
void drw_rect_border(Drw *drw, int x, int y, unsigned int w, unsigned int h) {
  if (!drw || !drw->scheme)
    return;
  XRenderColor rc = drw_xrender_color(drw_border_pixel(drw));
  XRenderFillRectangle(drw->dpy, PictOpOver, drw->picture, &rc, x, y, w, h);
}

/* Refill a solid color source only on color change. The pixel value is
 * stored as-is (non-premultiplied), matching how the rest of the bar is
 * painted directly with XFillRectangle/XFillArc. */
static void drw_rounded_refill(Drw *drw, Picture pic, unsigned long *cache,
                               unsigned long p, int r, int h) {
  if (*cache != p) {
    XRenderColor rc = drw_xrender_color(p);
    XRenderFillRectangle(drw->dpy, PictOpSrc, pic, &rc, 0, 0, r, h);
    *cache = p;
  }
}

/* Upload an 8-bit coverage buffer into pm and wrap it as an A8 picture. */
static void drw_rounded_makeimg(Drw *drw, Pixmap *pm, Picture *pic,
                                unsigned char *data, int r, int h) {
  GC gc = XCreateGC(drw->dpy, *pm, 0, NULL);
  XImage img = {r, h, 0, ZPixmap, (char *)data, ImageByteOrder(drw->dpy),
                BitmapUnit(drw->dpy), BitmapBitOrder(drw->dpy), 8, 8, 0, 8, 0,
                0, 0};
  XInitImage(&img);
  XPutImage(drw->dpy, *pm, gc, &img, 0, 0, 0, 0, r, h);
  XFreeGC(drw->dpy, gc);
  *pic = XRenderCreatePicture(
      drw->dpy, *pm, XRenderFindStandardFormat(drw->dpy, PictStandardA8), 0,
      NULL);
}

static int drw_rounded_impl(Drw *drw, int x, int y, unsigned int h, int radius,
                            int side, int outline, int bwidth) {
  static Pixmap ampm[2], cpm, bmpm[2], bpm;
  static Picture amask[2], color_pic, bmask[2], bpic;
  static unsigned char *fdata[2];
  static int cached_r = -1, cached_bw = -1;
  static unsigned int cached_h = 0;
  static unsigned long cached_pixel = ~0UL, cached_bpixel = ~0UL;
  int r, m;

  if (!drw || !drw->scheme)
    return 0;
  r = MIN(radius, (int)h / 2);
  if (r <= 0)
    return 0;

  /* Fill coverage masks, cached once per (r, h): amask[0] is the left cap,
   * amask[1] its horizontal mirror. Coverage is 0..255 by AA_SAMPLES x
   * AA_SAMPLES subsampling: the corner circle is centered at (r, r) for the
   * top arc and at (r, h - r) for the bottom arc, and the middle band is
   * fully covered. */
  if (cached_r != r || cached_h != h) {
    int ns = AA_SAMPLES * AA_SAMPLES;
    double rr = (double)r * r;

    if (color_pic != None) {
      XRenderFreePicture(drw->dpy, color_pic);
      XFreePixmap(drw->dpy, cpm);
    }
    if (bpic != None) {
      XRenderFreePicture(drw->dpy, bpic);
      XFreePixmap(drw->dpy, bpm);
    }
    if (fdata[0]) {
      free(fdata[0]);
      free(fdata[1]);
    }
    fdata[0] = ecalloc((size_t)r * h, 1);
    fdata[1] = ecalloc((size_t)r * h, 1);
    cpm = XCreatePixmap(drw->dpy, drw->root, r, h, 32);
    if (!cpm)
      return 0;
    color_pic = XRenderCreatePicture(
        drw->dpy, cpm, XRenderFindStandardFormat(drw->dpy, PictStandardARGB32),
        0, NULL);
    if (!color_pic) {
      XFreePixmap(drw->dpy, cpm);
      cpm = None;
      return 0;
    }
    bpm = XCreatePixmap(drw->dpy, drw->root, r, h, 32);
    if (!bpm)
      return 0;
    bpic = XRenderCreatePicture(
        drw->dpy, bpm, XRenderFindStandardFormat(drw->dpy, PictStandardARGB32),
        0, NULL);
    if (!bpic) {
      XFreePixmap(drw->dpy, bpm);
      bpm = None;
      return 0;
    }
    cached_pixel = ~0UL;
    cached_bpixel = ~0UL;

    for (m = 0; m < 2; m++) {
      if (amask[m] != None)
        XRenderFreePicture(drw->dpy, amask[m]);
      if (ampm[m] != None)
        XFreePixmap(drw->dpy, ampm[m]);
      if (bmask[m] != None)
        XRenderFreePicture(drw->dpy, bmask[m]);
      if (bmpm[m] != None)
        XFreePixmap(drw->dpy, bmpm[m]);

      for (int j = 0; j < (int)h; j++)
        for (int i = 0; i < r; i++) {
          int px = m ? r - 1 - i : i;
          if (j >= r && j < h - r) {
            fdata[m][(size_t)j * r + i] = 255;
          } else {
            int sx, sy, n = 0;
            double cy = (j < r) ? r : (h - r);
            for (sy = 0; sy < AA_SAMPLES; sy++)
              for (sx = 0; sx < AA_SAMPLES; sx++) {
                double dx = px + (sx + 0.5) / AA_SAMPLES - r;
                double dy = j + (sy + 0.5) / AA_SAMPLES - cy;
                if (dx * dx + dy * dy <= rr)
                  n++;
              }
            fdata[m][(size_t)j * r + i] =
                (unsigned char)((n * 255 + ns / 2) / ns);
          }
        }

      ampm[m] = XCreatePixmap(drw->dpy, drw->root, r, h, 8);
      if (!ampm[m])
        return 0;
      drw_rounded_makeimg(drw, &ampm[m], &amask[m], fdata[m], r, (int)h);
      if (!amask[m]) {
        XFreePixmap(drw->dpy, ampm[m]);
        ampm[m] = None;
        return 0;
      }
    }
    cached_r = r;
    cached_h = h;
    cached_bw = -1; /* force border rebuild for the new size */
  }

  /* Outline masks, cached per (r, h, bwidth): the bwidth-wide band just
   * inside the shape edge, antialiased by AA_SAMPLES subsampling. The signed
   * distance d to the arc/straight edge is negative inside, so a sample lies
   * on the border when -bwidth < d <= 0. The inner straight edge (shared with
   * the flat tab body) naturally falls outside the band, so neighbouring caps
   * join seamlessly without special-casing. */
  if (outline && cached_bw != bwidth) {
    unsigned char *bdata = ecalloc((size_t)r * h, 1);
    int ns = AA_SAMPLES * AA_SAMPLES;
    double rr = (double)r * r;
    double lo2 = (r > bwidth) ? (double)(r - bwidth) * (r - bwidth) : -1.0;

    for (m = 0; m < 2; m++) {
      if (bmask[m] != None)
        XRenderFreePicture(drw->dpy, bmask[m]);
      if (bmpm[m] != None)
        XFreePixmap(drw->dpy, bmpm[m]);
      bmpm[m] = None;
      bmask[m] = None;

      for (int j = 0; j < (int)h; j++)
        for (int i = 0; i < r; i++) {
          int px = m ? r - 1 - i : i;
          int n = 0;
          for (int sy = 0; sy < AA_SAMPLES; sy++)
            for (int sx = 0; sx < AA_SAMPLES; sx++) {
              /* mark samples inside the bwidth band along the shape edge */
              if (j >= r && j < (int)h - r) {
                double x = i + (sx + 0.5) / AA_SAMPLES;
                if (m ? x > r - bwidth : x < bwidth)
                  n++;
              } else {
                double cy = (j < r) ? r : (h - r);
                double dx = px + (sx + 0.5) / AA_SAMPLES - r;
                double dy = j + (sy + 0.5) / AA_SAMPLES - cy;
                double R2 = dx * dx + dy * dy;
                if (R2 <= rr && R2 > lo2)
                  n++;
              }
            }
          bdata[(size_t)j * r + i] = (unsigned char)((n * 255 + ns / 2) / ns);
        }

      bmpm[m] = XCreatePixmap(drw->dpy, drw->root, r, h, 8);
      if (!bmpm[m]) {
        free(bdata);
        return 0;
      }
      drw_rounded_makeimg(drw, &bmpm[m], &bmask[m], bdata, r, (int)h);
      if (!bmask[m]) {
        free(bdata);
        return 0;
      }
    }
    free(bdata);
    cached_bw = bwidth;
  }

  if (outline)
    drw_rounded_refill(drw, bpic, &cached_bpixel, drw_border_pixel(drw), r, h);
  else
    drw_rounded_refill(drw, color_pic, &cached_pixel,
                       drw->scheme[ColBg].pixel, r, h);

  m = (side == RoundedLeft) ? 0 : 1;
  XRenderComposite(drw->dpy, PictOpOver, outline ? bpic : color_pic,
                   outline ? bmask[m] : amask[m], drw->picture, 0, 0, 0, 0, x,
                   y, r, h);

  return r;
}

int drw_rounded(Drw *drw, int x, int y, unsigned int h, int radius, int side) {
  return drw_rounded_impl(drw, x, y, h, radius, side, 0, 0);
}

int drw_rounded_border(Drw *drw, int x, int y, unsigned int h, int radius,
                       int side, int bwidth) {
  if (bwidth <= 0)
    return 0;
  return drw_rounded_impl(drw, x, y, h, radius, side, 1, bwidth);
}

int drw_text(Drw *drw, int x, int y, unsigned int w, unsigned int h, unsigned int lpad, const char *text, int invert, int skip_pad) {
  int i, ty, ellipsis_x = 0;
  unsigned int tmpw, ew, ellipsis_w = 0, ellipsis_len;
  XftDraw *d = NULL;
  Fnt *usedfont, *curfont, *nextfont;
  int utf8strlen, utf8charlen, render = x || y || w || h;
  long utf8codepoint = 0;
  const char *utf8str;
  FcCharSet *fccharset;
  FcPattern *fcpattern;
  FcPattern *match;
  XftResult result;
  int charexists = 0, overflow = 0;
  /* keep track of a couple codepoints for which we have no match. */
  enum { nomatches_len = 64 };
  static struct {
    long codepoint[nomatches_len];
    unsigned int idx;
  } nomatches;
  static unsigned int ellipsis_width = 0;

  if (!drw || (render && (!drw->scheme || !w)) || !text || !drw->fonts)
    return 0;

  if (!render) {
    w = invert ? invert : ~invert;
  } else {
    XSetForeground(drw->dpy, drw->gc,
                   drw->scheme[invert ? ColFg : ColBg].pixel);
    if (w < lpad) {
      return x + w;
    }
    if (skip_pad)
      x += lpad;
    XFillRectangle(drw->dpy, drw->drawable, drw->gc, x, y, skip_pad?w-lpad:w, h);
    d = XftDrawCreate(drw->dpy, drw->drawable, drw->visual, drw->cmap);
    if (!skip_pad)
      x += lpad;
    w -= lpad;
  }

  usedfont = drw->fonts;
  if (!ellipsis_width && render)
    ellipsis_width = drw_fontset_getwidth(drw, "...");
  while (1) {
    ew = ellipsis_len = utf8strlen = 0;
    utf8str = text;
    nextfont = NULL;
    while (*text) {
      utf8charlen = utf8decode(text, &utf8codepoint, UTF_SIZ);

      for (curfont = drw->fonts; curfont; curfont = curfont->next) {
        charexists = charexists ||
                     XftCharExists(drw->dpy, curfont->xfont, utf8codepoint);
        if (charexists) {
          drw_font_getexts(curfont, text, utf8charlen, &tmpw, NULL);
          if (ew + ellipsis_width <= w) {
            /* keep track where the ellipsis still fits */
            ellipsis_x = x + ew;
            ellipsis_w = w - ew;
            ellipsis_len = utf8strlen;
          }

          if (ew + tmpw > w) {
            overflow = 1;
            /* called from drw_fontset_getwidth_clamp():
             * it wants the width AFTER the overflow
             */
            if (!render)
              x += tmpw;
            else
              utf8strlen = ellipsis_len;
          } else if (curfont == usedfont) {
            utf8strlen += utf8charlen;
            text += utf8charlen;
            ew += tmpw;
          } else {
            nextfont = curfont;
          }
          break;
        }
      }

      if (overflow || !charexists || nextfont)
        break;
      else
        charexists = 0;
    }

    if (utf8strlen) {
      if (render) {
        ty = y + (h - usedfont->h) / 2 + usedfont->xfont->ascent;
        XftDrawStringUtf8(d, &drw->scheme[invert ? ColBg : ColFg],
                          usedfont->xfont, x, ty, (XftChar8 *)utf8str,
                          utf8strlen);
      }
      x += ew;
      w -= ew;
    }
    if (render && overflow)
      drw_text(drw, ellipsis_x, y, ellipsis_w, h, 0, "...", invert, 0);

    if (!*text || overflow) {
      break;
    } else if (nextfont) {
      charexists = 0;
      usedfont = nextfont;
    } else {
      /* Regardless of whether or not a fallback font is found, the
       * character must be drawn. */
      charexists = 1;

      for (i = 0; i < nomatches_len; ++i) {
        /* avoid calling XftFontMatch if we know we won't find a match */
        if (utf8codepoint == nomatches.codepoint[i])
          goto no_match;
      }

      fccharset = FcCharSetCreate();
      FcCharSetAddChar(fccharset, utf8codepoint);

      if (!drw->fonts->pattern) {
        /* Refer to the comment in xfont_create for more information. */
        die("the first font in the cache must be loaded from a font string.");
      }

      fcpattern = FcPatternDuplicate(drw->fonts->pattern);
      FcPatternAddCharSet(fcpattern, FC_CHARSET, fccharset);
      FcPatternAddBool(fcpattern, FC_SCALABLE, FcTrue);
      FcPatternAddBool(fcpattern, FC_COLOR, FcFalse);

      FcConfigSubstitute(NULL, fcpattern, FcMatchPattern);
      FcDefaultSubstitute(fcpattern);
      match = XftFontMatch(drw->dpy, drw->screen, fcpattern, &result);

      FcCharSetDestroy(fccharset);
      FcPatternDestroy(fcpattern);

      if (match) {
        usedfont = xfont_create(drw, NULL, match);
        if (usedfont &&
            XftCharExists(drw->dpy, usedfont->xfont, utf8codepoint)) {
          for (curfont = drw->fonts; curfont->next; curfont = curfont->next)
            ; /* NOP */
          curfont->next = usedfont;
        } else {
          xfont_free(usedfont);
          nomatches.codepoint[++nomatches.idx % nomatches_len] = utf8codepoint;
        no_match:
          usedfont = drw->fonts;
        }
      }
    }
  }
  if (d)
    XftDrawDestroy(d);

  return x + (render ? w : 0);
}

void drw_pic(Drw *drw, int x, int y, unsigned int w, unsigned int h,
             Picture pic) {
  if (!drw)
    return;
  XRenderComposite(drw->dpy, PictOpOver, pic, None, drw->picture, 0, 0, 0, 0, x,
                   y, w, h);
}

void drw_map(Drw *drw, Window win, int x, int y, unsigned int w,
             unsigned int h) {
  if (!drw)
    return;

  XCopyArea(drw->dpy, drw->drawable, win, drw->gc, x, y, w, h, x, y);
  XSync(drw->dpy, False);
}

unsigned int drw_fontset_getwidth(Drw *drw, const char *text) {
  if (!drw || !drw->fonts || !text)
    return 0;
  return drw_text(drw, 0, 0, 0, 0, 0, text, 0, 0);
}

unsigned int drw_fontset_getwidth_clamp(Drw *drw, const char *text,
                                        unsigned int n) {
  unsigned int tmp = 0;
  if (drw && drw->fonts && text && n)
    tmp = drw_text(drw, 0, 0, 0, 0, 0, text, n, 0);
  return MIN(n, tmp);
}

void drw_font_getexts(Fnt *font, const char *text, unsigned int len,
                      unsigned int *w, unsigned int *h) {
  XGlyphInfo ext;

  if (!font || !text)
    return;

  XftTextExtentsUtf8(font->dpy, font->xfont, (XftChar8 *)text, len, &ext);
  if (w)
    *w = ext.xOff;
  if (h)
    *h = font->h;
}

Cur *drw_cur_create(Drw *drw, int shape) {
  Cur *cur;

  if (!drw || !(cur = ecalloc(1, sizeof(Cur))))
    return NULL;

  cur->cursor = XCreateFontCursor(drw->dpy, shape);

  return cur;
}

void drw_cur_free(Drw *drw, Cur *cursor) {
  if (!cursor)
    return;

  XFreeCursor(drw->dpy, cursor->cursor);
  free(cursor);
}
