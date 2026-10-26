# Hot reload tests

Each directory of `cases/` holds the successive versions of the sources of a program (`v0/`, `v1/`...)
and the output expected when it runs with `hl --hot-reload`.

```
haxe --cwd tests/hotreload --run Run --hl <path to hl> --haxe <path to haxe> [case...]
```

The tests are written for the `haxe6-incremental` branch of the compiler (`HAXE_STD_PATH` must point to its `std`).

## How a case runs

- The versions are copied one after the other to the same directory `out/<case>/src` and compiled through
  a compilation server (`--wait`/`--connect`), as when editing the files, with `-D hl-memo-check` so that a
  warm compilation that does not give the same bytecode as a cold one fails the case. `--no-server`
  compiles each version on its own.
- The program then runs with `hl --hot-reload v0.hl v1.hl ...`. `common/Main.hx` drives the steps: it calls
  `Test.check(0)`, then for each step `i` reloads `v<i>.hl` with `hl.Api.checkReload` and calls
  `Test.check(i)`, which every version defines. No timing, no file watching.
- The output (the program and the `[HotReload]` messages of the VM) is compared with `expected.txt`;
  a `*` in an expected line matches any word. The actual output is kept in `out/<case>/output.txt`.

Optional files of a case:

- `vN/Main.hx` replaces `common/Main.hx` (to drive the steps differently).
- `build.txt`: extra compiler arguments, one per line.
- `env.txt`: `NAME=value` lines set while the program runs (`HL_HOT_RELOAD_GLOBALS=...`).
- `KNOWN_FAILURE`: the case describes a behavior that is not implemented yet; its failure is reported but
  does not fail the run. The file says what is missing.

## Notes

- With `-dce std` (the default), the first use of a static of a standard class adds a field to its class
  object, which changes its layout (see `dce_std`). Hot reload builds should use `-dce no` until class
  objects can be upgraded.
