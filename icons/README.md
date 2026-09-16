# icons/

Tab icon fallback used when a client has no `_NET_WM_ICON`.

- `tab-fallback.svg` — source, from [Lucide](https://lucide.dev) (`circle-help`).
- `tab-fallback.png` — 64x64 white stroke on transparent, generated with:

  ```
  convert -background none -density 400 tab-fallback.svg -resize 64x64 \
      -gravity center -extent 64x64 png32:tab-fallback.png
  ```

  The bar scales it to `ICONSIZE` at load time, the same way a real
  `_NET_WM_ICON` off a client is scaled.

Lucide is licensed under the [ISC License](https://github.com/lucide-icons/lucide/blob/main/LICENSE),
which permits use, modification and redistribution provided the copyright
notice and permission notice are retained. The icon is white so it reads on a
dark bar; regenerate the PNG if your theme needs another colour.
