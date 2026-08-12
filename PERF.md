# Performance Log

## drawstatusbar: malloc → stack buffer (2026-08-12)

| Idea | Baseline → Result | Verdict | Why |
|------|-------------------|---------|-----|
| malloc/free → char[1024] stack buf | drawstatusbar 1188µs → 962µs avg | **kept** | -19.0%, far exceeds run-to-run noise. drawbar also dropped 14.2% as side effect. |

### Baseline (437 calls)
- drawstatusbar: 519.2ms total, 1188µs avg
- drawbar: 647.3ms total, 1321µs avg

### After (454 calls)
- drawstatusbar: 436.8ms total, 962µs avg
- drawbar: 577.1ms total, 1134µs avg

### Other measurements (not actionable)
- getfacts: 57µs avg — double-traversal concern was unfounded, negligible
- manage XInternAtom: not worth caching, infrequent calls
- drw_map XSync: expected cost, no room for improvement
