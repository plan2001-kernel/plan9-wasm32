#include <u.h>
#include <libc.h>

/*
 * Browser wasm32 process benchmark: fork + wait compared with
 * fork + exec(same binary) + wait. Each child exits immediately.
 * Run in the real 9front-wasm32 kernel with tools/test/wasmapp.
 *
 * Allocations are cumulative because WebAssembly.Memory cannot shrink.
 * Thus the labels are additional allocated MB, not exact linear memory
 * size; the latter may include allocator and stack overhead.
 * No synthetic process substitution is used here.
 */
enum { MB = 1024*1024, Iterations = 40, Repeats = 3 };
static uchar *held[3];

static void
grow(int which, int mb)
{
	int i, n;
	uchar *p;

	if(mb == 0)
		return;
	n = mb*MB;
	p = malloc(n);
	if(p == nil)
		sysfatal("benchspawn: malloc %d MB: %r", mb);
	for(i = 0; i < n; i += 4096)
		p[i] = i/4096;
	held[which] = p;
}

static void
measure(int doexec, int mb, int rep)
{
	int i, pid;
	vlong t0, elapsed;
	Waitmsg *w;

	/* One untimed warm-up for JIT/Worker startup and initial file access. */
	for(i = -1; i < Iterations; i++){
		if(i == 0)
			t0 = nsec();
		pid = fork();
		if(pid < 0)
			sysfatal("benchspawn: fork: %r");
		if(pid == 0){
			if(doexec){
				execl("/bin/benchspawn", "benchspawn", "-x", nil);
				sysfatal("benchspawn: exec: %r");
			}
			exits(nil);
		}
		w = wait();
		if(w == nil)
			sysfatal("benchspawn: wait: %r");
		if(w->pid != pid)
			sysfatal("benchspawn: wait got pid %d, expected %d", w->pid, pid);
		free(w);
	}
	elapsed = nsec() - t0;
	print("BENCH mode=%s allocated_mb=%d repeat=%d n=%d total_ns=%lld ns_per_op=%lld\n",
		doexec ? "forkexec" : "fork", mb, rep, Iterations,
		elapsed, elapsed/Iterations);
}

void
main(int argc, char **argv)
{
	int sizes[3], i, r, mb;

	if(argc > 1 && strcmp(argv[1], "-x") == 0)
		exits(nil);
	sizes[0] = 0;
	sizes[1] = 8;
	sizes[2] = 32;
	print("BENCH_BEGIN platform=plan9-wasm32 source=benchspawn n=%d repeats=%d\n", Iterations, Repeats);
	for(i = 0; i < 3; i++){
		mb = sizes[i];
		if(i != 0)
			grow(i, mb - sizes[i-1]);
		for(r = 0; r < Repeats; r++){
			/* Alternate the two modes; comparable within each heap size. */
			if(r % 2 == 0){
				measure(0, mb, r);
				measure(1, mb, r);
			}else{
				measure(1, mb, r);
				measure(0, mb, r);
			}
		}
	}
	print("BENCH_END\n");
	exits(nil);
}
