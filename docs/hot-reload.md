# HashLink hot reload: plan for the `hot-reload` branch

## Context

- Compiler: `hot-reload` branch of HaxeFoundation/haxe, based on `haxe6-incremental`. Steps 1 and 2 of phase 10 in `docs/incremental-compilation.md` are done (symbolic generation per class, linker shared by cold and warm builds, classes generated in parallel). The delta (step 3) is not done.
- VM: `hot-reload` branch, created from `master` of HaxeFoundation/hashlink, which is HL 2.0 (new JIT: `jit_emit.c`, `jit_regs.c`, `jit_x86_64.c`). The hot reload code is in `src/reload.c`.
- Decision: **no delta for now**. The VM reloads the full `.hl` produced by the compiler.
- Environment: Windows (developers and AI agents).

## Status

| Item | State |
|---|---|
| Hot reload on the HL 2.0 JIT | Done (see below) |
| 1. Cumulative reference state | Done |
| 2. Globals capacity | Done (`HL_HOT_RELOAD_GLOBALS`) |
| 3. Linear-time matching | Done |
| 4. Layout changes | Done: refused with the types and fields named |
| 5. Entry point run again | Done: the statics and `__init__` of new classes are initialized (compiler + VM, see item 5); added statics of existing classes wait for U1 |
| Detecting a file change | High resolution write time and size |
| Harness `tests/hotreload/` | Done, Linux only so far, not in CI |
| Heaps app | `tests/hotreload/heaps/run.py`: all steps pass, shaders included (with `heaps.patch`) |
| U1. Class objects | Done, with the mechanism of instances (`statics`, `statics_many`, `dce_std`, Heaps) |
| U2. Safe point and stack checks | Done: other threads stopped during the reload, all stacks checked (`statics_on_stack`, `threads`) |
| U0, U4, U8. Instances | Done: in place when the block has room, moved otherwise (`instance_fields`, `instance_refs`, `instance_in_place`, Heaps) |
| U3. Spare room in instances | Dropped: hot reload must not add memory to every object |
| U5. Virtuals | Done for the fields of virtuals; interfaces kept (`instance_fields`) |
| U6. Hook after upgrade | Done: `onHotReload()` (`hooks`) |
| U7. Enums | Done (`enums`, `enum_params`) |

**HL 2.0 JIT.** On `master`, the new JIT did not support hot reload: `hl_jit_patch_method` asserted, `hl_jit_reset` was empty, the previous module was ignored by `hl_jit_code` and `m->jit_ctx` kept a freed context. Now:

- a reload allocates a new JIT context for its module;
- the reload module calls the functions it did not compile through their current address: the *slot* of the function, the entry of `functions_ptrs` of the module that added it (`hl_module.hot_slots`, `CALL_FUN` / `LOAD_FUN` / static closures);
- the start of a replaced function is rewritten as `mov rax, slot; jmp [rax]` (functions are 16 bytes aligned), so the callers compiled before, the vtables and the live closures reach the latest version.

**Findings while testing.**

- The static methods of a class are fields of its class object (`$Foo`): adding a static method, or changing its signature, changes the layout of `$Foo`. Handled by U1.
- With `-dce std` (the default), the first use of something of a standard class can change its layout: a static (`Type.getClassName`, `dce_std`) or an instance field (the stack of `haxe.Exception` adds `__exceptionStack`, `dce_instance_field`). Both are handled (U1, U4).
- Adding an instance method is accepted when the virtual slots do not change; the live type keeps its proto, so the new method is not visible to reflection on it (U5).
- The JIT of a reload read `rt->compareFun` of a live type as a function index, while it is the address of the function once the prototype of the type is initialized: any comparison of strings in reloaded code crashed. A reload now initializes the prototype and calls the address (`tests/hotreload/cases/string_compare`).
- Shaders (Heaps): the source of a shader is the static `SRC`, and `_SHADER` caches the shader built from it. The initializer of a static with `@:hotReload` runs again after every reload (`static_hot_reload`); with the change of Heaps in `tests/hotreload/heaps/heaps.patch` (the hxsl macro puts `@:hotReload` on `SRC`, `SharedShader` keeps its source, and with `-D hot-reload` `Shader.initialize` builds the shader again when `SRC` changed), a changed shader source is used by the new shader instances.
- The global of a class or an enum is now identified by its name: its type hash changed with any type it references, which allocated a new global and lost the statics.

## What already works

`hash_fun` (`src/code.c`) hashes constants by value (ints, floats, strings), types by structure and calls by function signature, not by index. The index shifts introduced by the linker (numbering by first use) therefore do not trigger a mass re-JIT: only the functions that actually changed are compiled again.

## Work items

### 1. Cumulative reference state

**Problem.** `hl_module_patch(m1, code)` (`src/module.c`) always compares the new code with `m1`, the module loaded at startup. Neither `m1->hash`, nor `m1->code`, nor `m1->globals_size` is updated after a patch (see the comment "should be added to m1 functions for later reload?"). Consequences, on every reload:

- functions added by a previous reload are seen as new and injected again;
- globals added by a previous reload are not found by `hl_code_hash_remap_globals`; they are allocated again after `m1->globals_size` and then zeroed by the `memset`, so a static variable created during the session loses its value.

**Decision.** The reference state is initialized **once only**, when the module is loaded, then updated after each successful patch. No reinitialization per reload.

**To do.**

- A reference state (signatures and hashes of the functions, signatures of the globals, `globals_size`, mapping from function to its `functions_ptrs` slot) created in `hl_module_init` in hot reload mode.
- After a successful patch: record the added functions, the new hashes of the modified functions, the new globals and the new size of the globals.
- When a patch fails: the reference state is left unchanged.

**Test.** Chain several reloads that add a method, then a new class with a static variable (a new class adds a global: its class object); check that the method is injected only once and that the static keeps its value from one reload to the next. Adding a static variable to an *existing* class changes the layout of its class object (`$Foo`), which is handled by the instance upgrade (phase U1).

### 2. Globals capacity

**Problem.** The reserve is fixed at `HOT_RELOAD_EXTRA_GLOBALS` (4096) slots, for both `globals_data` and `globals_indexes`, and `gsize` is never checked against this limit in `hl_module_patch`: after enough reloads, writes go past the end of the buffer.

**To do.** Check the capacity before writing; when it is exceeded, refuse the reload with a clear message (restart required). Possibly make the reserve configurable.

### 3. Linear-time matching

**Problem.** Functions (`hl_module_patch`) and globals (`hl_code_hash_remap_globals`) are matched with nested loops: quadratic cost, noticeable on a project the size of SpaceCraft.

**To do.** Hash tables by signature in the reference state (item 1), built once and kept up to date.

### 4. Layout changes

**Problem.** When an object, struct or enum type changes layout, the patch only logs "Type %s has changed" and still applies the functions: new code runs on instances with the old layout.

**To do.** Detect layout changes **before** JIT-compiling and patching; until the instance upgrade (next section) handles a change, refuse the whole reload with a clear message naming the types and the reason, so that the agent knows a restart is needed.

### 5. Entry point run again

**Result.** The generated entry point starts with `Type.init()`, which returns `false` when the types are already registered; the entry point then returns after the initialization of the classes (`Type.initClass` / `initEnum` only allocate the class objects that do not exist yet) and before the `__init__` blocks, the static initializers and `main`. A reload therefore keeps the state, but skipped the initializers of the new statics.

**Done (compiler and VM).** The entry point no longer returns after the initialization of the classes. It runs the `__init__` blocks and the static initializers in the order of a cold start, each guarded by a bit that the VM gives by words of 32 bits:

```
var bits = is_init ? -1 : $hot_init_bits(type of $Foo, w);   // once per class and per word
if( bits & (1 << k) ) Foo.x = <init>;                         // static x is own field w * 32 + k of $Foo
...
if( is_init ) main();
```

- Generated with `-D hl-ver=2.0.0` or higher only (the default `hl-ver` of the standard library is still 1.16.0); before, and for HL/C, the entry point is unchanged.
- Word `-1` tells whether the class is new; it guards `__init__`.
- `hot_init_bits` is an optional native (`?std`), never called when the program starts: no cost for a normal run, and an older VM still loads the code. HL/C keeps the previous entry point.
- The VM answers from the reload whose entry point runs: all the bits for a class object type that is new, nothing otherwise; with U1, the bits of the statics added to an existing class.
- The VM calls the entry point with `hl_dyn_call_safe`: an exception in an initializer is reported (the following initializers do not run, as on a cold start) and the program goes on.

**Problem.** After the patch, the VM calls the entry point again ("will only update types"). With `haxe6-incremental`, the linker generates the entry point again at every compilation.

**To do.** Check what the generated entry point runs; make sure a reload runs neither the `__init__` blocks nor the static variable initializers, otherwise the game state is reset. If needed, split on the compiler side the initialization of the types (can run again) from the initialization of the statics (once only).

## Instance upgrade: adding and removing fields on live objects

Goal: accept a reload that adds, removes or changes fields of a class whose instances are alive, by rewriting these instances to the new layout. Each phase can ship on its own; until a phase covers a case, item 4 refuses the reload for that case.

Constraints that shape the design:

- The HL GC does not move objects, and it scans the native stacks conservatively: a reference held on a stack cannot be rewritten safely. Instances are therefore upgraded **in place** (same address), never moved, except in the optional phase U8.
- JIT code reads fields at fixed offsets: a frame still running old code on an upgraded instance reads the wrong offsets. The upgrade only happens at a safe point, with checks on the stacks (U2).
- In HL, the static variables of a class are fields of its class object (type `$Foo`, held by a global): adding a static to an existing class is a layout change, of the simplest kind (one instance).

### U0. Layout diff

For each object or struct type whose layout changed, compute the mapping from the old fields to the new ones, over the whole class hierarchy (a field added to a base class shifts the fields of every subclass):

- fields matched by `hashed_name` and same type: value copied to the new offset;
- added fields: initialized to their default value (zero, `null`, `0.0`, `false`);
- removed fields: value dropped;
- field whose type changed: treated as removed + added (no conversion);
- method changes only (proto): no data to move, only the type pointer and the runtime tables.

Output: an upgrade plan per type (old size, new size, list of copies), and a report printed on reload (types, added and removed fields). The plan is computed before anything is patched; any case not supported yet refuses the reload (item 4).

### U1. Class objects (static variables)

**Done**, with the mechanism of instances (U4, U8): a class object is the instance of its class object type, upgraded in place when its block has room and moved otherwise. No spare room. The statics kept (same name and type) are copied to their new offsets, the others are cleared, the new static methods get their closures (the bindings of the fields added), then the entry point initializes the added statics (`hot_init_bits` returns their bits). A static whose type changed is removed and added.

The frame of the entry point is on the stack while the program runs and holds the class objects it initialized: these values are dead (it only returns after `main` or the event loop), so the search of the stacks for the blocks to move skips it. It is found from the return address into the entry point, with its frame pointer just under it, to the next return address into JIT code.

The class object of each class is a single instance held by a global. In hot reload mode, class objects are allocated with spare room (see U3) so that a new layout fits in place. Upgrade: copy according to the U0 plan, set the new type, initialize the new statics. Initializers of new static variables: run only the initializers of the **added** statics, never the existing ones (consistent with item 5); this needs the compiler to expose them separately from the entry point.

First phase to deliver, because it covers the most common edit (adding a static) with a single instance and no heap walk.

### U2. Safe point and stack checks

**Done.** `hl_module_patch` stops the other threads (`hl_gc_world`, at their safe points, as for a collection) while it compares, compiles and patches the code and upgrades the live values; they run again before the entry point, which is code of the program. Before a layout changes, the reload is refused when a running function uses the type (the object types each function uses are recorded when it is compiled: its registers are freed after JIT), naming it: the stack of the current thread is walked, the stacks of the other threads, saved where they stopped, are scanned for return addresses into JIT functions. The entry point is not checked: it reads the class objects before it calls `main`, and only returns after. The search for references to the blocks to move covers every thread (`threads`).

- The reload runs at the existing `reload_check` point, with every other HL thread stopped (stop the world, as for a collection).
- Walk the stacks of all threads (the stack walk used by the HL2 profiler and call stack capture): if a frame belongs to a function that reads or writes fields of an upgraded type (known from the bytecode: `OField`, `OSetField`, `OGetThis`, `OSetThis` on that type or its subclasses), refuse the reload and name the function. In practice the main loop function must not touch upgraded types directly; the message tells the agent which function to move.

### U3. Spare room in instances (dev mode)

**Dropped.** Hot reload must not add memory to every allocated object. Instances are upgraded in place when their GC block has room (the GC rounds sizes), and moved otherwise (U8). Class objects use the same mechanism (U1).

In hot reload mode, object allocation reserves extra room after the fields (a configurable ratio with a minimum number of slots), so that added fields fit in place. Also use the real size of the GC block (`hl_gc_get_memsize`), which is often larger than requested. Instances allocated before a layout change keep their spare room; instances allocated after it use the new size plus spare room. When an instance has no room left, the reload is refused (or relocated, U8).

### U4. Heap walk and in-place rewrite

**Done**, with U8. A type of an object whose fields change (same superclass, same overridden methods) is upgraded, with its subclasses (their fields move).

- Before anything is changed: a full collection, then a walk of the live blocks finds the instances (their first word is one of the `hl_type*` that share the previous layout: the live type, and the aliases of the reloads) and the virtuals. An instance whose block has room is rewritten in place, the others are moved; the stacks (above the call to `checkReload` for the current thread, and the registers it was called with) must not reference an instance to move, and no running function may use a type whose layout changes (U2): otherwise the reload is refused.
- Applied after the JIT, with the collections disabled: the fields kept (same name and type, through the whole hierarchy) are copied to their new offsets, the others cleared (default value). Then every `hl_type*` of the previous layout gets the new one (`obj`, `vobj_proto`, `mark_bits`): **instances keep their type pointer**, so `Std.isOfType`, `__type__` of the class object and the `super` of other types stay valid.
- The signature of a function no longer depends on the layout of the types of its arguments (only their names): a method of a type whose layout changes is replaced (the previous code jumps to the new one) instead of being seen as removed and added, so the live closures and vtables call the new body.
- The VM prints `[HotReload] N instances upgraded, M moved (R references updated)`.

- Enumerate the live blocks of the GC (`gc_iter_pages` / `gc_iter_live_blocks`, as the memory dump does) and keep the objects whose type pointer is an old type to upgrade (the type pointer is the first word of an object).
- Check that every instance fits (U3) before modifying any of them: the upgrade is all or nothing.
- For each instance: copy the fields to a temporary buffer, write them back at the new offsets, initialize the added fields, clear the removed ones (the GC must not see stale pointers), set the new type pointer.
- Run after the new functions are JIT-compiled and before their pointers are patched, so that no code runs between the two.

### U5. Derived runtime structures

- **Virtuals** (`vvirtual`): their field pointers (`hl_vfields`) point into the object. Virtuals are heap blocks too: during the walk of U4, recompute the field pointers of the virtuals whose value is an upgraded object, or drop the cached virtual when a field it exposes was removed.
- **Runtime type data** (`hl_runtime_obj`: field indexes, dynamic lookup table, interface and virtual caches, bindings): rebuilt for the new type; `hl_flush_proto` already handles the protos.
- **Instance closures**: they hold the object pointer, which does not change; nothing to do, but the method they call follows the patch of the functions.

### U6. Haxe hook after upgrade

**Done.** After a reload that upgraded objects, once the entry point initialized the new statics and the other threads run again: the method `onHotReload()` (no argument) of each upgraded object is called once, and the static method `onHotReload()` of each class whose statics changed. An exception is reported (`[HotReload] Exception in onHotReload: ...`) and the other hooks still run. The closures are built while the objects are known to be alive, and kept in a rooted array until they are called. The name is a convention of the VM: with `-dce full`, the methods need `@:keep` (a `@:hotReload` metadata on the compiler side could keep them).

After a successful upgrade, call an optional method on each upgraded instance (for example `@:hotReload function onHotReload()`, name to be agreed with the compiler side), and an optional static method per class. Lets the game code initialize new fields with real values (resources, derived data) instead of defaults. Exceptions thrown by the hook are reported, not fatal.

### U7. Enums

**Done.** A changed enum is upgraded like the types of objects (its values are GC blocks whose first word is the type): the index of each live value is remapped to the constructor of the same name; the params of a constructor are kept by name when their type is the same (bytecode 7, written by the compiler from `hl-ver=2.0.0`, has the names of the params; by position for older bytecode), so that inserting, moving or removing a param keeps the data of the others; a renamed or new param gets the default value; a value is rewritten in place when its block has room, moved otherwise. Every `hl_type*` of the enum gets the new `tenum` and mark bits. The array `__evalues__` of the enum object, which the entry point reads to set the globals of the constant values again, is built in the order of the new constructors with the live constants (their identity is kept: `fav == Green` stays true). The standard library (`Type.initEnum`, `hot-reload` branch of Haxe) builds `__constructs__` and `__emap__` again when the constructors changed (`Type.getEnumConstructs`, `Type.createEnum`).

A removed constructor with params and live values refuses the reload. The constant of a removed constructor without params is referenced by the enum object and a global: the reload is refused only when something else references it (a walk of the blocks, the stacks); otherwise it is detached.

The signature of a local function keeps the structure of its context (an anonymous enum): when the captured variables change, the live closures keep their body and their context, the new closures use the new body (`closure_captures`). Otherwise the live closures run the new body of their function, which is identified by its name in its method rather than by its rank (bytecode 7): the compiler names a local function after what receives it (a variable, a field, the function it is passed to, `return`), prefixed by the name of the local function that holds it and a dot (`maker.inner`), with a rank among the functions of the same name (`closure_insert`). Adding a local function only shifts the ones of the same name.

- Constructor added at the end: no change for live values.
- Constructor added in the middle, removed or reordered: remap the constructor index of every live value of that enum (found by the U4 walk); values of a removed constructor have no target, so the reload is refused if any is alive.
- Arguments of a constructor changed: same rules as fields (U0), applied to the live values of that constructor.

### U8. Relocation (optional, last)

**Done** (moved before U3, which is dropped). The new block is allocated with the new size and the pair is added to the remaps of the GC (`hl_gc_remap_add`); `hl_gc_remap_apply` runs a collection whose marking, parallel like any collection, replaces the references to the previous blocks when it scans a block (precisely with the mark bits, conservatively for raw blocks) and the roots: the previous blocks are no longer marked and are freed by the same collection. The marking is instantiated twice, the usual one has no test of the remaps. Then the fields of the virtuals are computed again, and the object maps (`hl_ho*`, which hash the address of their keys), registered when allocated in hot reload mode, are rehashed. A reference from a stack refuses the reload.

The change of the types of the live instances (`obj`, `vobj_proto`, `mark_bits` of every alias) is in one function, `hot_set_layout`: the representation of the types will change.

**Measures** (`HL_HOT_RELOAD_TIMES` prints the phases of a reload; `instance_many`, Linux, 4 cores): 200 000 instances moved, with a linked list, references between them and an object map of 50 000 keys: the walk that finds the instances 30–40 ms (a full collection, then a walk of the blocks on one thread), the allocation and copy 16–21 ms, the remap collection 24 ms with one marking thread and 13 ms with four, the object maps 5 ms. The walk and the copy could be made parallel too.

When an instance has no spare room: allocate the new instance, copy, then redirect the references. Heap references are found by the U4 walk and rewritten precisely; references from globals as well. References from stacks cannot be rewritten (conservative scan): if the old address appears on any stack, refuse the reload. Kept for last because U3 should cover most cases in dev mode.

### Tests per phase

- U1: add, remove and retype a static on an existing class across several reloads; existing statics keep their values.
- U2: a reload changing a type used by a function active on the stack is refused with the function name.
- U4/U5: instances of a class and of its subclasses, referenced from fields, arrays, maps, closures and virtuals (structural typing), survive the addition and removal of fields in the base class.
- U6: the hook runs once per upgraded instance.
- U7: live enum values after adding, reordering and removing constructors.
- U7: live enum values after moving, inserting and renaming params (`enum_params`).

## Test matrix

Every operation a developer or an agent can make between two reloads, with the expected result. **Accepted** means the reload applies and the program behaves as if started with the new code, except for the state kept by design (existing values of fields and statics). **Refused** means the reload is rejected with a message naming the cause, and the program keeps running the previous code unchanged. The *Needs* column gives the item or phase that makes the case pass; a case whose phase is not done must be refused, never corrupt memory.

### Harness

- Location: `tests/hotreload/` (see its README). One directory per case, with the successive versions of the sources (`v0/`, `v1/`, `v2/`...) and the expected output.
- The runner compiles every version to its own `.hl` (through the compilation server of `haxe6-incremental`, as in real use), then runs `hl --hot-reload v0.hl`.
- The program drives the steps itself: at step *i* it calls `hl.Api.checkReload("v<i>.hl")` (the `debugFile` argument loads another file), then calls `Test.check(i)`, which is defined in each version and therefore patched. No timing, no file watching: the test is deterministic.
- `check_reload` compares the last write time in the best resolution of the file system (100 ns on NTFS) and the size.
- Each case checks: the result of the reload (accepted or refused, and the message), the values of the live state after it, the behavior of the new code, and that a second and third reload still work (cumulative state, item 1).
- Run on Windows (JIT x86_64) in CI, plus `-D hl-memo-check` on the compiler side to make sure warm and cold `.hl` are identical.

### Methods

| Operation | Expected | Needs |
|---|---|---|
| Change the body of an instance method | Accepted; new body on the next call | now |
| Change the body of a static method | Accepted | now |
| Add an instance method (called from changed code) | Accepted, injected once across reloads | 1 |
| Add a static method | Accepted (a static method is a field of the class object) | done (U1) |
| Remove a method no longer called | Accepted; old code stays for live closures | now |
| Change the signature (arguments, return type) | Accepted as remove + add; callers recompiled | 1 (U1 for a static method) |
| Rename a method | Accepted as remove + add | 1 |
| Add an override in a subclass | Accepted; proto updated, existing instances dispatch to it | U0, U5 |
| Remove an override | Accepted; dispatch falls back to the parent | U0, U5 |
| Change an `inline` function | Accepted; the callers are compiled again by the compiler | now |
| Change a method called through an interface, `Dynamic` or reflection | Accepted | now |
| Change a method held by a live closure (`obj.method` stored in a field) | Accepted; the closure calls the new body | now |
| Change a function active on the stack (main loop) | Accepted for the next call; the running frame finishes the old body | now |
| Change a `@:generic` method or a method of a `@:generic` class | Accepted; each instance patched | now |

### Local functions and closures

| Operation | Expected | Needs |
|---|---|---|
| Change the body of a local function | Accepted; new closures use it | now |
| Add a local function before existing ones | Accepted; live closures run the new body of their own function (matched by name) | done |
| Remove a local function | Accepted; live closures keep the old body | now |
| Change the captured variables | Accepted for new closures; live closures keep their old environment | done |

### Instance fields

| Operation | Expected | Needs |
|---|---|---|
| Add a field | Accepted; live instances get the default value | done |
| Remove a field | Accepted; value dropped, GC no longer sees it | done |
| Change the type of a field | Accepted as remove + add (default value) | done |
| Reorder fields | Accepted; values kept | done |
| Rename a field | Accepted as remove + add | done |
| Add a field to a base class with live subclass instances | Accepted; subclass fields shifted, values kept | done |
| Field read through a structure or typedef (virtual) | Accepted; virtual field pointers updated | done |
| Field read through `Reflect`/`Dynamic` | Accepted; lookup tables rebuilt | done |
| Add a property (`get`/`set`) | Accepted | U0, U4 |
| Change the initializer of an existing field | Accepted; live instances keep their value, new ones use it | now |
| Instance whose block has no room | Moved, references updated; refused if a stack references it | done |
| Type of a field used by a function active on the stack | Refused, function named | done (current thread) |
| Upgrade hook `onHotReload` | Called once per upgraded instance | done |

### Static variables

| Operation | Expected | Needs |
|---|---|---|
| Add a static to an existing class | Accepted; only its initializer runs | done |
| Remove a static | Accepted | done |
| Change the type of a static | Accepted as remove + add | done |
| Change the initializer of an existing static | Accepted; the current value is kept | 5 |
| Change a `static inline` / `final` constant | Accepted; inlined uses compiled again; a static keeps its value, unless it has `@:hotReload` (set again by every reload) | done |
| Static of a new class | Accepted; value kept across later reloads | 1 |

### Classes and interfaces

| Operation | Expected | Needs |
|---|---|---|
| Add a class (new module) | Accepted; usable from changed code and `Type.resolveClass` | 1 |
| Add a class in an existing module | Accepted | 1 |
| Remove a class without live instances | Accepted | 1 |
| Remove a class with live instances | Refused | U0 |
| Rename a class | Accepted as remove + add (same rule for live instances) | 1, U0 |
| Change the superclass | Refused while instances are alive | U0 |
| Add an interface to a class | Accepted; `Std.isOfType` and casts see it | U5 |
| Remove an interface | Accepted; cached virtuals for it dropped | U5 |
| Add a method to an interface (and its implementations) | Accepted | U5 |
| New exception class, thrown and caught | Accepted | 1 |
| Class with `__init__` changed | Accepted; `__init__` not run again | 5 |

### Enums

| Operation | Expected | Needs |
|---|---|---|
| Add a constructor at the end | Accepted | done |
| Add a constructor in the middle | Accepted; live values remapped | done |
| Reorder constructors | Accepted; live values remapped | done |
| Remove a constructor without live values | Accepted | done |
| Remove a constructor with live values | Refused | done |
| Change the arguments of a constructor | Accepted; params kept by position and type | done |
| Add an enum | Accepted | 1 |
| Change an `enum abstract` value | Accepted; uses are inlined and compiled again; stored values unchanged | now |
| `switch` on an enum after a change | Accepted; matches the new indexes | done |

### Other

| Operation | Expected | Needs |
|---|---|---|
| Change an abstract (methods, operators) | Accepted; inlined uses compiled again | now |
| Add a field to a typedef used by live virtuals | Accepted | U5 |
| New string, int, float or bytes constants | Accepted | now |
| New native function (`@:hlNative`, new hdll) | Accepted; library loaded on reload | 1 |
| Change `main` | Accepted; `main` is not run again | 5 |
| Reload while worker threads run | Accepted at the safe point, threads stopped | done |
| Reload with no change | Reported as no change, nothing patched | now |
| Many successive reloads (100+) | Accepted; no growth beyond capacity, no lost state | 1, 2 |
| Revert to an earlier version | Accepted like any other change | 1 |
| Two versions written within the same second | Seen as a change | now |
| Compilation error (no new `.hl`) | Nothing happens | now |
| Truncated or partly written `.hl` | Refused, nothing patched | now |
| Large project (SpaceCraft) | Reload time measured and tracked | 3 |

## Testing on a Heaps app

Heaps checks for a reload every frame when compiled with `-D hot-reload` (`hxd.System`, which calls `sys_check_reload` and then `hxd.System.onReload`). Run the app with `hl --hot-reload app.hl` and compile it through the compilation server of `haxe6-incremental`; the `[HotReload]` messages of the VM say what was applied or why a reload was refused.

- The main loop (`hxd.System.mainLoop`) stays on the stack: it keeps running its old body, which is fine as long as it does not change.
- `tests/hotreload/heaps/run.py` drives such an app (see `tests/hotreload/README.md`). On a small app, with the whole of Heaps compiled (`-dce no`), on Linux: a warm compilation takes 0.7 to 1 s (2.3 s after an edit of `hxd.Math.clamp`, inlined in many places), and the reload is applied in less than 0.15 s, at the next frame.
- Covered: method bodies of the app and of Heaps, a new class with statics, inline functions of Heaps (37 functions compiled again), a field added to a live `h2d.Text` (refused, then reverted), quick successive edits, `hxd.System.onReload` called after each reload.
- To do: reload time on a large app (SpaceCraft), Windows, shaders (see the findings above).

## Open points

- **Detecting a file change.** `check_reload` (`src/main.c`) now compares the high resolution write time and the size. A partly written file fails to load and is read again at its next change; the compiler should still write the `.hl` atomically (temporary file, then rename).
- **Closures of the same name.** Local functions are identified by their name in their method (bytecode 7, `hl_function.lname`), by their rank before. Two functions of the same name (two closures passed to `map` in the same method) are told apart by their rank: adding one before the other shifts it.

## Out of scope for now

- Bytecode delta (step 3 of phase 10) and delta loader.
- Memory snapshot/restore.
- Direct channel from the compile server to the VM (Windows named pipe) instead of watching the file.
