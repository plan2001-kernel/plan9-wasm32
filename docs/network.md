# The machine's network

A machine in a browser has an Ethernet card (`#l0`, `etherwasm32.c`)
whose wire is a WebSocket to its page's origin, `/ether`
(`docs/kernel.md`).  A frame is one binary WebSocket message each way.
The link connects at boot: there is no passkey, no ticket and no network
prompt.

## Local

`tools/serve` relays the page's `/ether` (the next port on loopback) to
`tools/gateway`, the other end of the wire.  `tools/gateway` is a switch
and DHCP server:

    tools/gateway            # the Internet through passt, no root
    tools/gateway -n         # a network of its own, 10.0.2.0/24, no way out (tests)
    tools/gateway -t tap0    # a TAP made by root, bridged or NATed to the LAN

Its DHCP hands out 10.0.2.15 and up by MAC, router 10.0.2.2, and DNS
(`GATEWAY_DNS`, default 1.1.1.1 and 8.8.8.8).

## plan2001.com

The public host is Ubuntu Linux (`docs/plan2001-server.md`).  nginx
proxies `wss://plan2001.com/ether` to `tools/gateway` on
`127.0.0.1:17080`.  The gateway opens `tap0` (10.0.2.2/24); the host
enables forwarding and masquerades 10.0.2.0/24 out its default
interface.  Every machine started in a browser on the site gets DHCP,
DNS, a gateway and NAT to the Internet, all by default; many machines
can run at once.

The firmware always connects to its own origin's `/ether`
(`wss://` on https, or the local relay on loopback): a page pointed at
another gateway would be a man in the middle.  nginx passes the
browser's `Origin`, and the gateway refuses an origin it was not started
with (`-o https://plan2001.com`).

## Trust

The machine's page is its firmware; an installed machine's firmware comes
from its own disk (`index.html` and `platform.js` on its 9fat), which its
owner can change.  Any of it runs with the page's origin's rights.  So
the origin holds nothing that is not the machine's own: its disk (OPFS)
and its BIOS settings.  It no longer holds a network ticket: there is
none.

The old public design (a separate `auth.plan2001.com` origin, passkeys,
invitations and `aux/ethernetd`) is retired and is not used on the
Ubuntu host.

## Tests

`tools/test/run ethergw` (only when named) tests `aux/ethernetd` and
`aux/etherws` on `tools/gateway -n`; `tools/test/run net` and
`tools/test/etherclient` test the switch, DHCP, ARP, ping and NAT.  The
server's gateway is the same `tools/gateway`, with a TAP and NAT
(`tools/site/ubuntu/gateway-net-up`).
