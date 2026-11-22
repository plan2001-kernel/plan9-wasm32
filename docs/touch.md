# Touch: one finger, Rio's own menus

A phone's screen is the machine's mouse (`platform.js`).  A finger is
Plan 9's button 1; a physical mouse is unchanged, chords included.
There is no permanent button bar: the page has no m1/m2/m3
selector.  A small floating control strip (keyboard, fullscreen and the
special keys a phone lacks) is at the top; see **Keyboard** below.

## Tap, double tap, drag

A tap sends an ordinary button 1 down and up at the tapped point; it is
not a menu gesture.  Rio's own window and text logic then does what a
mouse click does:

- the first tap on an unfocused window tops and focuses it (Rio
  consumes that click, so no text is selected);
- a tap in the focused window moves the text cursor;
- a drag past a small slop is button 1 down at the first point, a move
  for each move, and button 1 up: ordinary Rio text selection;
- two quick taps are two ordinary clicks, so Rio's own double-click
  code (`wind.c`: 500 ms and 3 pixels) selects the word under a tap on
  the text.  A double tap on a blank (space or tab) or in the empty left
  or right margin selects that line instead.  The page has no separate
  word or line selection algorithm.

The page sends each tap at release.  Rio's own click timing and
text-boundary interpretation are unchanged; a double tap never becomes
a drag or a long press because it stays inside the slop and below the
long-press time.

A line selection uses Rio's input boundary, `w->qh`, when it lies on
that line.  On the current editable input line `qh` is after the
shell's prompt output and before the user's input, so a touch double
tap on blank space or a margin selects the input without the prompt.
A prompt-only line therefore gives an empty selection.  Nothing
matches or hardcodes `term% `: any shell's prompt is excluded the same
way.  On a historical output line `qh` is not on the line, so the line
is selected as it is; Rio cannot tell which of its old characters were
prompts.  A wrapped input line is one logical line, and both its rows
share the same prompt boundary.  This is touch-only; a physical mouse
keeps its original double-click behavior.

## Long press: button 2

A finger held still for `LONGPRESS` (500 ms) sends `LONGREQ` instead of
a button 1 click.  Rio hit-tests the point with its own `wpointto` and
opens the ordinary button 2 menu of that text window (cut, paste,
snarf, plumb, look, send, scroll) through `touchmenuhit`.  The menu is
`genmenuhit`, not a copy; its synthetic `Mouse` is held down only in
memory.  Releasing the finger does not close it: the next independent
tap selects an item, and a tap outside returns -1 and only dismisses
it.  A long press on the keyboard window, a window whose program has
opened the mouse (`w->mouseopen`), or a border does nothing.

## Gray desktop: button 3

After an ordinary tap, the page also sends `MENUREQ`.  Rio opens the
persistent button 3 menu (new, resize, move, delete, hide, hidden
windows) only when the tap is on the gray desktop (`wpointto` nil).
In a window `MENUREQ` does nothing, so an ordinary tap does not become
a button 2 menu.

The tap that selects a touch menu is followed by that tap's own
`MENUREQ`; Rio skips it (`touchmenureqskip`) so selecting an item does
not immediately open another menu.  A tap outside the menu dismisses
it the same way.

## Window actions

New, Resize, Move, Delete and Hide are Rio's ordinary button 3
operations.  When the menu was opened by touch, the following gesture
is accepted as button 1 as well as button 3 (`actionbutton` in
`rio.c`), so a finger can sweep the new window's rectangle or drag and
resize a window.  A `MENUREQ` that belongs to such a window action is
suppressed so it does not open a second menu.

## The touch bits

`platform.js` marks finger events with `Touchbit` (1<<6), a tap's
desktop menu request with `Menureq` (1<<7), a long press with
`Longreq` (1<<17), and the first button 1 down of a gesture with
`Touchstart` (1<<18).  `devmouse.c` keeps those bits in `/dev/mouse`
(they are not part of the three buttons or the wheel).  Rio strips them
before a window or program sees a `Mouse`; `Touchstart` is how a Rio
text window knows a double click came from a finger for the margin
rule above, and it never reaches an application.  A physical mouse
never sets them.  A program that reads `/dev/mouse` itself sees the
extra high bits and may ignore them; Rio never forwards them.

## Keyboard

The phone's system keyboard is the hidden `#kbd` field.  The **KBD**
control at the top of the page shows and hides it explicitly:

- the first tap focuses `#kbd` and calls `navigator.virtualKeyboard.show()`
  where the VirtualKeyboard API exists;
- the second tap calls `navigator.virtualKeyboard.hide()` and blurs the
  field;
- no touch on the Rio canvas focuses the field.  A tap or drag there is
  only Rio's mouse, so menus and text selection are not interrupted.

The field keeps its existing input forwarding: characters, Backspace,
Return and Unicode text go to `#b/kbd` exactly as before.

## Keyboard overlay

`navigator.virtualKeyboard.overlaysContent = true` and the page's
`interactive-widget=overlays-content` viewport make the Android keyboard
draw over the page instead of resizing it.  The guest framebuffer is set
once at boot and is never changed by the keyboard: Rio's windows keep
their coordinates, and the browser document does not scroll.  The
controls stay at the top, above the keyboard.

## Fullscreen

The **Full** control calls `document.documentElement.requestFullscreen()`
from the click itself (the user gesture), and `document.exitFullscreen()`
when already fullscreen.  The canvas remains `100vw` by `100vh`, so Rio
fills the display; `platform.js` still maps pointer coordinates through
the canvas's bounding rectangle, so the mapping does not change.  The
keyboard never changes the framebuffer size in either mode.

## Limitations

- Some Android Chrome versions leave fullscreen when the software
  keyboard is shown, although the page asked for overlays.  The
  **KBD** control still works; use the page as an installed standalone
  PWA (Add to Home screen) when a device does that, so the browser's
  fullscreen and the keyboard do not fight.  The page declares a
  `manifest.webmanifest` with `display: standalone` for this case.
- A tap is classified and sent when the finger lifts, so its click time
  is the release time.  Rio's own 500 ms and 3 pixel double-click rules
  still decide word or line selection.
- Move and Resize first ask which window (Rio's `pointto`), then take
  the drag; a touch therefore uses a tap on the window and then the
  drag.
- A physical mouse is not menu-driven automatically; its buttons and
  chords are as they were.
