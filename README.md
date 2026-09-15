# tundra-ip

Low-level network configuration tool, developed for Tundra OS, works on any linux — manages links,
addresses, and routes by speaking netlink directly to the kernel.

`tundra-ip` is Tundra's own equivalent of `ip` (iproute2): the tool that
configures and inspects the network at layer 1. It reads and modifies
interfaces, addresses, and routing tables.

## Syntax philosophy

`tundra-ip` follows **its own syntax**, which is the canonical way of using
the tool and the one documented here.

Additionally, it understands a subset of common `ip` (iproute2) syntax that
external packages expect. This compatibility layer exists so a symlink
(`ip` -> `tundra-ip`) can satisfy `ip`-dependent software without installing
iproute2 alongside. The `ip`-compatible forms are documented separately in
[Compatibility with ip-dependent packages](#compatibility-with-ip-dependent-packages).

## Requirements

- Linux (uses `NETLINK_ROUTE`).
- Read commands (`show`, `status`) run as a normal user.
- Write commands (`add`, `del`, `flush`, `up`, `down`, `set`) require root.

## Usage

### status

A unified, per-interface view of the network (links + addresses + routes).

```
tundra-ip status [--verbose] [--local]
```

- `--verbose`: show all obtainable information per interface.
- `--local`: include the `local` routing table.

### addr

```
tundra-ip addr show [--verbose]
tundra-ip addr add   <IP>/<prefix> on <interface>
tundra-ip addr del   <IP>/<prefix> on <interface>
tundra-ip addr flush on <interface>
```

### route

```
tundra-ip route show [--verbose] [--local]
tundra-ip route add   <net>/<prefix> [via <gateway>] on <interface> [metric <n>]
tundra-ip route add   default via <gateway> on <interface> [metric <n>]
tundra-ip route del   <net>/<prefix> [on <interface>]
tundra-ip route del   default [on <interface>]
tundra-ip route flush on <interface> [--all]
```

- `default` is a destination shorthand for `0.0.0.0/0`.
- `via <gateway>` makes the route go through a gateway; without it, the route
  is directly connected.
- `route flush` clears the `main` table by default; `--all` clears the
  interface's routes in every table.

### link

```
tundra-ip link show [--verbose]
tundra-ip link up      <interface>
tundra-ip link down    <interface>
tundra-ip link set mac <mac> on <interface>
```

## Flags

| Flag        | Meaning                                            |
|-------------|----------------------------------------------------|
| `--verbose` | Show detailed / full information (show and status) |
| `--local`   | Include the `local` routing table (route, status)  |
| `--all`     | Flush routes in all tables (route flush)           |
| `--version` | Print version and exit                             |
| `--help`    | Print usage and exit                               |

Flags may appear in any position on the command line.

## Notes on syntax

- **`on <interface>`** is the canonical preposition for "through this
  interface" (where `ip` uses `dev`).
- **`set mac`** is canonical for changing a MAC address (where `ip` uses
  `set address`).

## Compatibility with ip-dependent packages

> **Status: planned, not yet implemented.**

For software that invokes `ip` directly, `tundra-ip` is intended to accept the
following `ip`-style forms (for use via an `ip` -> `tundra-ip` symlink, not as
the recommended way to use the tool):

```
ip route add default via <gateway> dev <interface>
ip link set <interface> up
ip link set <interface> down
ip link set <interface> address <mac>
```

## Currently unsupported

Planned but not yet implemented:

- IPv6 write operations.
- The `ip`-compatibility layer described above.
- `link add` / `del` (virtual interfaces).
- `link set mtu`.
- `route set-metric`.
- Meta flags like --version and --help
