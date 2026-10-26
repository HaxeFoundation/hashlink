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

## Heaps app

`heaps/run.py` runs a Heaps app (`heaps/src`) compiled with `-D hot-reload -dce no`: Heaps checks for a reload
every frame. For each step, the driver edits the app or a copy of Heaps, compiles through a compilation
server and waits for the status the app prints, or for the refusal of the reload.

```
python3 tests/hotreload/heaps/run.py --hl <hl> --haxe <haxe> --heaps <heaps> --format <format> [--gdb]
```

- Heaps must compile with the compiler: the `haxe6-annotations` branch of Heaps for `haxe6-incremental`.
- The app needs the `sdl`, `fmt` and `ui` libraries of HashLink next to `hl` (or in the library path).
- On Linux without a display, the app runs in `xvfb-run`. `--gdb` runs `hl` in gdb and prints the native
  stack on a crash.
- A step marked `known` describes a behavior that is not implemented yet: it is reported, not counted.
