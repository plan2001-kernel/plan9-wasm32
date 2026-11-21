#include <u.h>
#include <libc.h>

/*
 * spawn(2) (sysproc.c): ohjelma omaksi prosessikseen ilman forkia.
 * Ilman argumentteja testit (tools/test/run spawn); -b N [ohjelma args]:
 * kierroksittain N kertaa fork+exec+wait ja spawn+wait, ms/kerta.
 */

static void
reap(int pid, char *what)
{
	Waitmsg *w;

	w = wait();
	if(w == nil){
		print("%s: no child: %r\n", what);
		return;
	}
	print("%s: %s, status '%s'\n", what, w->pid == pid ? "pid ok" : "pid wrong", w->msg);
	free(w);
}

static void
fails(char *what, char *file, char **argv, int *fd, int nfd, int flag)
{
	if(spawn(file, argv, fd, nfd, flag) >= 0){
		print("%s: no error\n", what);
		return;
	}
	print("%s: -1 '%r'\n", what);
}

static void
tests(void)
{
	char *echo[] = { "echo", "spawned", "with", "args", nil };
	char *pecho[] = { "echo", "through the pipe", nil };
	char *none[] = { "nonexistent", nil };
	char *rc[] = { "rc", "-c", nil, nil };
	char buf[128], cmd[128];
	int pid, p[2], ops[4], n, cfd, ofd;

	pid = spawn("/bin/echo", echo, nil, 0, 0);
	reap(pid, "basic");

	/* lapsen stdout putkeen: dup(p[1], 1), close(p[0]) */
	if(pipe(p) < 0)
		sysfatal("pipe: %r");
	ops[0] = p[1];
	ops[1] = 1;
	ops[2] = -1;
	ops[3] = p[0];
	pid = spawn("/bin/echo", pecho, ops, 4, 0);
	close(p[1]);
	n = readn(p[0], buf, sizeof buf - 1);
	close(p[0]);
	buf[n > 0 ? n : 0] = 0;
	print("pipe: read '%s'\n", buf);
	reap(pid, "pipe");

	/* CCEXEC: OCEXECin kuvaaja suljetaan lapsessa, muut jäävät */
	cfd = open("/dev/null", OREAD|OCEXEC);
	ofd = open("/dev/null", OREAD);
	snprint(cmd, sizeof cmd, "test -e '#d/%d' && echo cexec open || echo cexec closed; test -e '#d/%d' && echo other open", cfd, ofd);
	rc[2] = cmd;
	pid = spawn("/bin/rc", rc, nil, 0, 0);
	reap(pid, "cexec");
	close(cfd);
	close(ofd);

	/* virheet kutsujalle, ei lasta */
	fails("nofile", "/bin/nonexistent", none, nil, 0, 0);
	fails("notprogram", "/rc/lib/rcmain", none, nil, 0, 0);
	fails("rfmem", "/bin/echo", echo, nil, 0, RFMEM);
	ops[0] = 99;
	ops[1] = 1;
	fails("badfd", "/bin/echo", echo, ops, 2, 0);
	fails("oddfd", "/bin/echo", echo, ops, 1, 0);
	pid = spawn("/bin/echo", echo, nil, 0, 0);
	reap(pid, "after the errors");
}

/* liput: RFNOWAIT, nimiavaruus ja ympäristö jaettuna ja omana */
static void
flagtests(void)
{
	char *quiet[] = { "echo", "-n", nil };
	char *bindrc[] = { "rc", "-c", "bind /bin /boot", nil };
	char *envrc[] = { "rc", "-c", "echo -n child >/env/spawnenv", nil };
	char *v;
	int pid, i;

	if(spawn("/bin/echo", quiet, nil, 0, RFNOWAIT) < 0)
		print("nowait: spawn: %r\n");
	sleep(300);
	if(wait() == nil)
		print("nowait: no child to wait for: %r\n");
	else
		print("nowait: a child to wait for\n");

	for(i = 0; i < 2; i++){
		pid = spawn("/bin/rc", bindrc, nil, 0, i ? RFNAMEG : 0);
		reap(pid, i ? "namespace RFNAMEG" : "namespace shared");
		print("namespace %s: /boot/rc %s\n", i ? "RFNAMEG" : "shared", access("/boot/rc", AEXIST) == 0 ? "there" : "not there");
		if(!i)
			unmount("/bin", "/boot");
	}

	for(i = 0; i < 2; i++){
		remove("/env/spawnenv");
		pid = spawn("/bin/rc", envrc, nil, 0, i ? RFENVG : 0);
		reap(pid, i ? "env RFENVG" : "env shared");
		v = getenv("spawnenv");
		print("env %s: spawnenv %s\n", i ? "RFENVG" : "shared", v != nil ? v : "unset");
		free(v);
	}
}

/* n lasta yhtä aikaa (sleep 1), kaikki odotetaan; muistirajan yli ne voivat päättyä virheeseen */
static void
manytests(char *what, int n, int sum)
{
	char *sl[] = { "sleep", "1", nil };
	Waitmsg *w;
	int i, made, reaped, ok;

	made = 0;
	for(i = 0; i < n; i++)
		if(spawn("/bin/sleep", sl, nil, 0, 0) >= 0)
			made++;
	reaped = ok = 0;
	while(reaped < made && (w = wait()) != nil){
		reaped++;
		if(w->msg[0] == 0)
			ok++;
		free(w);
	}
	if(sum)
		print("%s: %d spawned, %d reaped, %d ok\n", what, made, reaped, ok);
	else
		print("%s: %d spawned, %d reaped%s\n", what, made, reaped, made == n && reaped == n ? ", all" : "");
}

/* n käynnistystä (fork+exec+wait tai spawn+wait): ms/kerta */
static double
round1(int n, int usespawn, char *file, char **argv)
{
	vlong t0;
	int i, pid;

	t0 = nsec();
	for(i = 0; i < n; i++){
		if(usespawn){
			if((pid = spawn(file, argv, nil, 0, RFREND)) < 0)
				sysfatal("spawn: %r");
		}else switch(pid = rfork(RFPROC|RFFDG|RFREND)){
		case -1:
			sysfatal("fork: %r");
		case 0:
			exec(file, argv);
			exits("exec");
		}
		while(waitpid() != pid)
			;
	}
	return (nsec()-t0)/1e6/n;
}

static int
dcmp(void *a, void *b)
{
	double x = *(double*)a, y = *(double*)b;

	return x < y ? -1 : x > y;
}

/*
 * Kierroksia Rounds, kussakin n kumpaakin; järjestys vuorottelee
 * kierroksittain (lämpeneminen ei suosi kumpaakaan).  Raakaluvut ja mediaani.
 */
enum { Rounds = 6 };

static void
bench(int n, char *file, char **argv)
{
	double f[Rounds], s[Rounds];
	int k;

	for(k = 0; k < Rounds; k++){
		if(k & 1){
			s[k] = round1(n, 1, file, argv);
			f[k] = round1(n, 0, file, argv);
		}else{
			f[k] = round1(n, 0, file, argv);
			s[k] = round1(n, 1, file, argv);
		}
		print("%s round %d (%s first): fork+exec %.2f ms, spawn %.2f ms\n", file, k, k&1 ? "spawn" : "fork", f[k], s[k]);
	}
	qsort(f, Rounds, sizeof f[0], dcmp);
	qsort(s, Rounds, sizeof s[0], dcmp);
	print("%s: median fork+exec %.2f ms, spawn %.2f ms (%d rounds of %d)\n", file,
		(f[Rounds/2-1]+f[Rounds/2])/2, (s[Rounds/2-1]+s[Rounds/2])/2, Rounds, n);
}

void
main(int argc, char **argv)
{
	char *echo[] = { "echo", "-n", nil };

	if(argc > 2 && strcmp(argv[1], "-b") == 0){
		if(argc > 3)
			bench(atoi(argv[2]), argv[3], argv+3);
		else
			bench(atoi(argv[2]), "/bin/echo", echo);
		exits(nil);
	}
	tests();
	flagtests();
	manytests("concurrent", 30, 1);
	manytests("over the memory limit", 90, 0);
	exits(nil);
}
