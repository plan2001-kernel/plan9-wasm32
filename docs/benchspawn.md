# Process creation performance benchmark (browser wasm32)

This benchmark measures the **current real 9front-wasm32 implementation**, not
a simulated spawn syscall. Run on a Linux build host with plan9port, 7z,
bubblewrap and Chromium, or via the branch-specific GitHub Actions workflow.

Run from the repository root:

```sh
PLAN9=$HOME/plan9port tools/test/run echo
SITE="$PWD/build/test/site" SERIAL=1 HALTWAIT=540 \
  ARGS='["/bin/benchspawn"]' tools/test/wasmapp benchspawn
grep '^BENCH ' build/test/benchspawn.txt
```

The benchmark measures 40 sequential fork/wait/exit operations and 40
fork/exec/wait operations per repetition, repeated three times each at
approximately 0, 8 and 32 MiB of **cumulative additional allocated heap**.
Each mode starts with one unmeasured fork. Every child exits with no work,
or execs the same minimal benchmark module with `-x` and exits. A PID
mismatch or a failed fork/exec/wait aborts the run.

Important: `allocated_mb` is cumulative requested heap, **not** exact
WebAssembly linear-memory size. The JS fork clones the entire
`mem.buffer.byteLength`, including stack and unused pages.
The `forkexec` number includes the first fork's unwind/rewind,
copy, worker creation, wasm module loading/compilation/instantiation,
exec and wait. This is deliberate: it is the overhead a future
spawn syscall would avoid only in part.

A future direct `spawn` implementation should be measured under the **same**
conditions and in the same Chromium version. It should skip the parent's
unwind, address-space copy and child's rewind. It still needs a new Proc,
Worker, program loading and module instantiation unless those are separately
optimized. A module cache and worker pool are independent optimizations
that should be benchmarked before attributing all acceleration to spawn.

Compare median ns/op from the three repeats, plus growth from 0 to 32 MiB.
A direct spawn speedup cannot be claimed until a real implementation exists.
