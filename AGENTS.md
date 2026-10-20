# AGENTS.md — working agreement for AI agents in this repo

## C style (must follow)

This fork uses 1TBS braces everywhere, including function definitions.
Do NOT write K&R-style definitions with the return type, name, or brace
on their own lines.

```c
/* good: signature on one line, brace attached */
int titleh(Client *c) {
```

```c
/* bad: K&R definition, return type / brace on their own lines */
int
titleh(Client *c)
{
```

- 2-space indent, no tabs.
- Keep the surrounding code's column alignment; do not reformat
  unrelated lines (no drive-by reformatting).
- Bound actions take `(const Arg *arg, unsigned int event)`; internal
  call sites pass `KeyPressMask` explicitly.

## Verify (must run before finishing)

- Build: `make clean && make` — must be warning-free.
- Test: `make test` — must be fully green.
- `git diff --check` — must be clean.
- `git diff --stat` — no unrelated files, no debug leftovers.
