/*
 * tools/test/run riotouch: one finger in a Rio text window.  The window
 * is at -r -10 0 390 300 (TOUCH=1: a 400x700 phone), its text frame at
 * x=11, y=5, and the default 8x16 font's rows 16 pixels apart.  Rio's
 * riotest=1 writes each touch double click's range and text to
 * /tmp/riotest, which the test's rc cats after these gestures.
 */
(() => {
	const c = document.getElementById("screen");
	const Control = "\uf017";
	let id = 1;
	const tap = (x, y) => {
		const p = id++, ev = (t, b) => c.dispatchEvent(new PointerEvent(t, {
			pointerId: p, pointerType: "touch", clientX: x, clientY: y,
			buttons: b, bubbles: true, cancelable: true }));
		ev("pointerdown", 1);
		setTimeout(() => ev("pointerup", 0), 30);
	};
	const dbl = (x, y) => {
		tap(x, y);
		setTimeout(() => tap(x, y), 120);
	};
	const key = (r) => window.plan9.kbd("r" + r + "\0R" + r + "\0");
	const ctrl = (r) => window.plan9.kbd("r" + Control + "\0r" + r + "\0R" + r + "\0R" + Control + "\0");
	const type = (s) => { for (const r of s) key(r); };
	const at = (s, f) => setTimeout(f, s * 1000);
	const digits = "0123456789".repeat(6);

	at(20, () => tap(100, 100));		/* focus the window */
	at(20.5, () => ctrl("b"));		/* the output point, after the prompt */
	at(21, () => type("echo alpha beta"));
	at(22, () => dbl(384, 10));		/* right margin: current input, no prompt */
	at(23, () => key("\n"));
	at(24.5, () => dbl(384, 10));		/* the command's historical line */
	at(25.5, () => dbl(384, 26));		/* its output, alpha beta */
	at(26.5, () => dbl(384, 42));		/* the new prompt-only line */
	at(27.5, () => dbl(25, 26));		/* a word: alpha */
	at(28.5, () => dbl(9, 26));		/* the left margin: alpha beta */
	at(30, () => ctrl("b"));		/* after the prompt again */
	at(30.5, () => type("echo " + digits));
	at(31.5, () => key("\n"));
	at(33.5, () => dbl(9, 74));		/* left margin of the wrapped output */
})();
