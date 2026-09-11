# spacepilot-bridge

Firmware for a Raspberry Pi Pico (RP2040) that sits between an old 3Dconnexion
**SpacePilot** and a computer, and makes the computer see a **SpaceMouse Pro
Wireless**.

```
 SpacePilot  ──USB──▶  Raspberry Pi Pico  ──USB──▶  computer
 046d:c625             PIO USB host on            sees 256f:c631
                       GPIO0/1 + native USB       "SpaceMouse Pro Wireless (cabled)"
```

Why: 3Dconnexion dropped the SpacePilot family from 3DxWare years ago, and the
old devices also split their reports differently from current ones.  The bridge
speaks the old protocol on one side and the current one on the other, so the
current drivers (3DxWare on Windows/macOS, spacenavd on Linux) just work.

Status: **tested and working with a SpacePilot (046d:c625)**.  The other USB
devices from [spacenavd](https://github.com/FreeSpacenav/spacenavd)'s table are
recognised too, but none of them has been tried — see
[Testing status](#testing-status) if you own one.

## What it does

| Feature | Notes |
| --- | --- |
| 6 axes | Translation (report 1) and rotation (report 2) of the old device are merged into the single 6-axis report of the SpaceMouse Pro. |
| Buttons | Mapped through a compile-time table (`src/button_maps.h`).  The SpacePilot has 21 keys, the SpaceMouse Pro has 15; keys with a counterpart on the SpaceMouse Pro are mapped to it by default, the rest are ignored until you map them. |
| LED | The LED output report sent by the driver is forwarded to the source device. |
| Identity | `256f:c631` "3Dconnexion SpaceMouse Pro Wireless (cabled)", the same identity the [AndunHH/spacemouse](https://github.com/AndunHH/spacemouse) project uses successfully with 3DxWare and spacenavd. |
| Source devices | All 21 USB devices in spacenavd's table (below).  Any other HID *multi-axis controller* is accepted with the generic V3DK button map.  Keyboards and mice plugged into the host port are ignored. |
| Not done | The SpacePilot (Pro) LCD is not driven (it lives on a separate vendor-specific interface and is simply left alone).  No battery report, the device claims to be the cabled variant.  spacenavd's *serial* devices (Spaceball 1003/2003/3003/4000, Magellan SpaceMouse, serial CadMan) are RS-232, not USB, and are out of scope. |

### Source devices

Every USB device from spacenavd's `src/dev.c`.  Per device the bridge knows
two things: whether its axes need normalising (spacenavd's `DF_SWAPYZ |
DF_INVYZ`, which only three early Logitech-era devices lack) and how its keys
are numbered.

| Device | USB ID | Axes | Keys |
| --- | --- | --- | --- |
| SpaceMouse Plus XT | `046d:c603` | normalised | sequential |
| CadMan | `046d:c605` | | sequential |
| SpaceMouse Classic | `046d:c606` | normalised | sequential |
| Spaceball 5000 | `046d:c621` | | sequential |
| Space Traveller | `046d:c623` | | sequential |
| **SpacePilot** (tested) | `046d:c625` | | own table |
| SpaceNavigator | `046d:c626` | | sequential |
| SpaceExplorer | `046d:c627` | | sequential |
| SpaceNavigator for Notebooks | `046d:c628` | | sequential |
| SpacePilot Pro | `046d:c629` | | V3DK |
| SpaceMouse Pro | `046d:c62b` | | V3DK |
| NuLOOQ | `046d:c640` | normalised | sequential |
| SpaceMouse Wireless (cabled / receiver / BT) | `256f:c62e` `c62f` `c63a` | | sequential |
| SpaceMouse Pro Wireless (cabled / receiver / BT) | `256f:c631` `c632` `c638` | | V3DK |
| SpaceMouse Enterprise | `256f:c633` | | V3DK |
| SpaceMouse Compact | `256f:c635` | | sequential |
| SpaceMouse Module | `256f:c636` | | sequential |

*V3DK*: the device numbers its keys with 3Dconnexion's key codes, the same
ones the SpaceMouse Pro uses, so shared keys map 1:1.  *Sequential*: the
device numbers its keys 0, 1, 2, … in layout order; the n-th key becomes the
n-th SpaceMouse Pro key.  Exactly right for the two-button devices, a
best-effort default for the rest — see `src/button_maps.h` for how this was
derived from spacenavd and how to correct it.

Only the SpacePilot is the project's target and the only device that has been
verified on hardware.  The others cost nothing but a table row; whether their
axes and keys come out right depends on feedback from people who own them.

## Hardware

One Raspberry Pi Pico and a USB extension cable cut in half — the same build as
the single-Pico [HID Remapper](https://github.com/jfedor2/hid-remapper/blob/master/HARDWARE.md):

| Cable wire (female / device end) | Pico pin |
| --- | --- |
| D+ (green) | GPIO0 (pin 1) |
| D- (white) | GPIO1 (pin 2) |
| VBUS (red) | VBUS (pin 40) |
| GND (black) | GND (pin 38) |

The Pico's own micro-USB port goes to the computer.  See [docs/HARDWARE.md](docs/HARDWARE.md)
for details, the debug UART and power considerations (the SpacePilot with its
backlit LCD is not a low-power device).

## Building

Prerequisites: CMake ≥ 3.13, `gcc-arm-none-eabi` (with newlib), a host C
compiler for the tests.  The Pico SDK and Pico-PIO-USB come in as submodules.

```sh
git clone https://github.com/dewi-ny-je/spacepilot-bridge
cd spacepilot-bridge
git submodule update --init                      # pico-sdk + Pico-PIO-USB
git -C lib/pico-sdk submodule update --init lib/tinyusb

cmake -S . -B build -DPICO_BOARD=pico            # or pico_w
cmake --build build
# -> build/spacepilot_bridge.uf2
```

An existing SDK checkout can be used instead of the submodule by setting
`PICO_SDK_PATH` (and `PICO_PIO_USB_PATH`) in the environment.  The first build
also compiles `picotool` from the SDK to produce the UF2.

Flash it the usual way: hold BOOTSEL while plugging the Pico into the computer,
then copy `spacepilot_bridge.uf2` onto the `RPI-RP2` drive.

Run the unit tests of the translation core on the host with:

```sh
make -C test
```

## Configuration

Everything is compile time.

* `src/config.h` — USB identity, report timing, axis mapping / inversion / gain,
  dead zone, pins, debug level.  Every value is `#ifndef`-guarded, so it can
  also be overridden without editing the file:

  ```sh
  cmake -S . -B build -DBRIDGE_EXTRA_DEFS="-DBRIDGE_DEBUG=2;-DBRIDGE_USB_PID=0xc632"
  ```

* `src/button_maps.h` — the button tables.  One line per key:

  ```c
  { V3DK_5, SMP_1 },   // SpacePilot Pro key "5" acts as SpaceMouse Pro key "1"
  ```

  Names are in `src/buttons.h`.

## Testing status

**SpacePilot (046d:c625): tested, works** — enumeration, all six axes, the
button table and driver acceptance have been verified on real hardware.

**Every other device in the table: untested.**  Their entries are derived from
spacenavd's source code, not from a device on a desk, so they need feedback
from actual users.  If you own one, this is what to check, in order, and what
to report (an issue with the UART log and the device name is ideal):

1. **Enumeration on the host port.**  Set `BRIDGE_DEBUG` to 1 (default) and
   watch the UART on GPIO16/17 at 115200 baud.  You should see
   `source attached: <device name> [vid:pid]`.
2. **Axes.**  With `BRIDGE_DEBUG=2` every raw report is printed.  If an axis
   feels swapped or inverted, the device's `BRIDGE_SRC_FIX_YZ` flag in
   `src/button_maps.h` is probably wrong for it (see `docs/PROTOCOL.md` for
   how it was chosen); `BRIDGE_AXIS_MAP` / `BRIDGE_AXIS_INVERT` in
   `src/config.h` override it either way.
3. **Buttons.**  Pressing a key prints its raw bit number.  For the "V3DK"
   devices the numbering is well established; for the "sequential" ones the
   physical key order is a guess, so expect to reorder `map_sequential` (or
   give the device its own table).
4. **Driver acceptance.**  3DxWare should list a "SpaceMouse Pro Wireless".

The translation logic is deliberately decoupled from TinyUSB so that anything
you find can be reproduced in `test/test_bridge.c` without hardware.

## How it was put together

* The SpacePilot (Pro) side — USB IDs, axis conventions, button numbering — was
  derived from [spacenavd](https://github.com/FreeSpacenav/spacenavd)
  (`src/dev.c`, `doc/spnavrc_spilot`).  spacenavd itself does not parse raw USB
  reports on Linux (the kernel does), so the raw report layout is the classic
  3Dconnexion one documented in [docs/PROTOCOL.md](docs/PROTOCOL.md).
* The SpaceMouse Pro Wireless side — HID report descriptor, report cadence and
  the "send a few zero reports when coming to rest" behaviour that 3DxWare
  expects — follows [AndunHH/spacemouse](https://github.com/AndunHH/spacemouse)
  (`SpaceMouseHID.cpp/.h`).
* The RP2040 dual-role USB setup (TinyUSB device on the native port, TinyUSB
  host on a [Pico-PIO-USB](https://github.com/sekigon-gonnoc/Pico-PIO-USB)
  port, binary copied to RAM) follows [HID Remapper](https://github.com/jfedor2/hid-remapper).

No code was copied from those projects; this repository is licensed under the
Apache License 2.0 (see `LICENSE`).  The SDK and Pico-PIO-USB submodules carry
their own licenses.

## Layout

```
src/main.c            init, main loop, status LED
src/bridge.[ch]       translation core (no hardware dependencies)
src/config.h          user settings
src/button_maps.h     button mapping tables
src/buttons.h         symbolic button names
src/usb_descriptors.c the emulated SpaceMouse Pro Wireless
src/usb_device.c      TinyUSB device callbacks (native USB -> computer)
src/usb_host.c        TinyUSB host callbacks (PIO USB <- SpacePilot)
src/tusb_config.h     TinyUSB configuration
test/                 host-side unit tests of the translation core
docs/                 hardware and protocol notes
```
