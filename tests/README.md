# Connect Four regression tests

Requires Python 3 and GCC or Clang. Run from the folder containing `connect4.c`:

```sh
cc -std=c99 -Wall -Wextra connect4.c -o connect4
./connect4
```

Run the external regression suite:

```sh
python3 tests/run_tests.py
CC=gcc python3 tests/run_tests.py
CC=clang python3 tests/run_tests.py
CC=clang CFLAGS="-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer" ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1 python3 tests/run_tests.py
```

`CC`, `CPPFLAGS`, `CFLAGS`, and `LDFLAGS` are honored. `TEST_TIMEOUT` controls
the per-process timeout in seconds (default 5). Use `--source /path/connect4.c`
to test another copy. Binaries use an automatically cleaned temporary folder;
the game source is never edited. No third-party Python modules are needed.
The sanitizer command keeps address/undefined checks enabled and disables leak
checking for compatibility with sandboxed processes and macOS.

Coverage includes ordinary and middle-joining horizontal/diagonal wins,
vertical wins, both colors, all board edges/corners, runs of one through five,
opponent/empty gaps, 3,000 deterministic random boards against an independent
oracle, full-column retry without changing player, invalid numbers and letters,
oversized/malformed tokens, whitespace-separated moves,
EOF at several input stages, and an independently validated legal 81-cell draw.
The 80-move draw prefix must remain unfinished. Completed wins/draws must stop
at the correct board, and draw tests reject an extra turn prompt. A legal
81st-move win must take priority over a draw.

`win_harness.c` includes the game after renaming its `main` and calls its original
board/win functions. The Python tests compile both programs and exercise the
real terminal interface, checking final boards and outcomes. Execution time
and output are bounded so regressions to the original input loop fail safely.
