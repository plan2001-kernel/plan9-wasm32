# plan2001.com server (Ubuntu Linux)

**Owner-confirmed 2026-11-03; the browser service was restored 2026-11-13.**

plan2001.com runs **Ubuntu Linux**, not Plan 9.  The old Plan 9 host,
rc-httpd/tlssrv routing, rcpu/9pterm administration, WireGuard tunnel and
the native `aux/ethernetd` passkey gateway are retired.  The files
`tools/site/deploy`, `tools/site/server.rc`, `tools/site/site.rc` and the
invitation scripts describe that retired host and are not used here.

## What is served

nginx (`tools/site/ubuntu/nginx-plan2001.conf`) serves from
`/var/www/plan2001`:

- `/` — the simple front page (`tools/site/front.html`), with a link to
  the browser machine.
- `/plan9-wasm32/` — the launcher (`tools/site/index.html`): download the
  ISO, power on, BIOS settings (boot order and devices), Help.
- `/plan9-wasm32/boot/` — the machine's firmware (`kernel.html` as
  `index.html`, `platform.js`), its kernel, boot file system and
  `plan9.ini`.  `plan9.ini` boots the server's ISO by default
  (`cdboot=yes`, `iso=/iso/plan9-wasm32.iso`); a disk with an installed
  system boots from its own 9fat and firmware instead.
- `/iso/plan9-wasm32.iso` — the distribution, with HTTP Range requests.
- `/ether` — WebSocket upgrade to the virtual Ethernet gateway.

`/plan9-wasm32/` responses carry `Cross-Origin-Opener-Policy: same-origin`
and `Cross-Origin-Embedder-Policy: require-corp` (the machine's memory is
a SharedArrayBuffer).  The ISO is never gzipped: the firmware needs its
true length and 206 responses.

## The virtual Ethernet

`tools/gateway` (installed at `/opt/plan9-wasm32/gateway`) is a switch and
DHCP server.  It listens on `127.0.0.1:17080`; nginx proxies `/ether` to
it.  It opens `tap0`; `gateway-net-up` configures the host:

- `tap0` 10.0.2.2/24, `ip_forward=1`;
- `tools/gateway -t tap0 -d`: each machine gets 10.0.2.15 and up by its
  MAC, router 10.0.2.2, and DNS 1.1.1.1 and 8.8.8.8 (DHCP option 6);
- iptables NAT: `10.0.2.0/24` masqueraded out the default interface, with
  FORWARD ACCEPTs inserted before any catch-all REJECT (Oracle's image has
  one).

There is **no passkey and no netauth=**: the machine's firmware connects
the card to `wss://plan2001.com/ether` at boot, and the gateway accepts it.
All running machines share the host's uplink through NAT.

`tools/gateway` is the same switch used locally by `tools/serve`; the
server adds the TAP, NAT and systemd (`tools/site/ubuntu/`).

## Deploying

From this repository, with the host in `~/.ssh/config` and passwordless
sudo there:

    tools/site/ubuntu/deploy plan2001.com

It stages `build/site-ubuntu` from `build/dist` (the ISO) and the sources
(launcher, firmware), uploads it, installs nginx and the
`plan9-gateway.service` unit, and reloads both.  `build/live` must exist
(`tools/build`) for the kernel and boot file system, and
`build/dist/plan9-wasm32.iso` (`tools/dist`) for the ISO.

Plan9-wasm32 itself remains a browser guest OS.  Local builds,
`tools/serve` and `tools/gateway` are development tools; the web server
does not run the guest operating system.
