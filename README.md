# spacepilot-bridge

An interoperability study: **to what extent is a twenty-year-old 3Dconnexion
6DOF device still protocol-compatible with what current driver software
expects?**

The experiment takes the form of firmware for an RP2040 board, which sits
between an old **SpacePilot** and a computer and re-expresses the old device's
input reports in the format used by a current **SpaceMouse Pro Wireless**, so
that the degree of compatibility can actually be measured.

```
 SpacePilot  ──USB──▶  Raspberry Pi Pico  ──USB──▶  computer
 046d:c625             PIO USB host on            sees 256f:c631
                       GPIO0/1 + native USB       "SpaceMouse Pro Wireless (cabled)"
```

**The question.**  3Dconnexion's 6DOF devices span two decades and more than
one protocol generation; the older models were dropped from the current driver
software some years ago, and they also divide their input reports differently
from current ones.  Is that gap fundamental, or does it come down to a handful
of report-layout details?  The only way to find out is to build the translation
and see how far it gets: the firmware converts between the two generations'
report formats, and the experiment is then to observe whether current drivers
(3DxWare on Windows/macOS, spacenavd on Linux) accept the result and interpret
it correctly.

**The result so far.**  For the one device examined — a SpacePilot (046d:c625),
on a Waveshare RP2040-Zero — the gap turns out to be small and entirely
mechanical: the axes, the buttons and the LED all map across, and current
drivers accept the translated reports.  The other USB devices in
[spacenavd](https://github.com/FreeSpacenav/spacenavd)'s table are described by
the same model but have not been examined — see
[Testing status](#testing-status) if you own one and want to extend the
comparison.

## Scope and intent

This project exists to explore **how much the 3Dconnexion 6DOF devices have in
common**, and to keep one old device working as a side effect.  Two decades
separate the Logitech-branded SpacePilot from today's SpaceMouse Enterprise,
yet they share a single protocol lineage: the same HID report layout, the same
±350 logical axis range, the same key numbering with gaps left where a model
has fewer keys, the same LED output report.  Once that is written down
([docs/PROTOCOL.md](docs/PROTOCOL.md)), making an old device speak as a current
one turns out to be mostly a table — the differences reduce to two per-device
columns, axis convention and key numbering, both of which can be read straight
out of spacenavd's own device table.  The 21-device list below is really the
result of the exercise: it is a map of where the family agrees and where it
does not.

**Research and private experimentation only — no commercial redistribution.**
What this repository contains is a compatibility finding and the apparatus used
to obtain it, not a product, and it should not be treated as one: please do not
sell it, or hardware running it.  Besides being the author's wish, there are
concrete reasons:

* The identity the firmware presents is the *variable under test*.  To learn
  whether a current driver accepts the translated reports at all, the device
  has to claim 3Dconnexion's USB vendor ID (`0x256f`), the product ID of a
  current model and the matching product-name strings — there is no way to ask
  the question otherwise.  USB vendor IDs are assigned to their owner;
  reproducing the experiment on your own bench is one thing, putting such a
  device in front of anyone else is quite another.
* The method necessarily presents a current product's identity to that
  product's own driver.  That is defensible as an interoperability
  investigation on hardware you own; it is not a basis for a product.
* [AndunHH/spacemouse](https://github.com/AndunHH/spacemouse), one of the
  projects this one draws its knowledge of the SpaceMouse Pro Wireless from,
  is published under CC BY-NC-SA 4.0 — explicitly non-commercial.

Anyone wanting to build and ship hardware should obtain their own USB vendor ID
and write their own report descriptor, at which point none of the identity
choices made here apply to them anyway.

**No affiliation.**  This project is independent and is not affiliated with,
endorsed by, or connected to 3Dconnexion in any way.  "3Dconnexion",
"SpacePilot", "SpaceMouse" and "3DxWare" are trademarks of their respective
owner, used here only to identify the hardware and software under study.  No
vendor software was decompiled and none is redistributed: every protocol detail
recorded in [docs/PROTOCOL.md](docs/PROTOCOL.md) was derived from published
open-source projects — spacenavd, AndunHH/spacemouse, 3dxdisp — or from
observing the behaviour of a device the author owns.

## What it does

| Feature | Notes |
| --- | --- |
| 6 axes | Translation (report 1) and rotation (report 2) of the old device are merged into the single 6-axis report of the SpaceMouse Pro. |
| Buttons | Mapped through a compile-time table (`src/button_maps.h`).  The SpacePilot has 21 keys, the SpaceMouse Pro has 15; keys with a counterpart on the SpaceMouse Pro are mapped to it by default, the rest are ignored until you map them. |
| LED | The LED output report sent by the driver is forwarded to the source device. |
| Identity | `256f:c631` "3Dconnexion SpaceMouse Pro Wireless (cabled)" — the variable under test, and the same identity [AndunHH/spacemouse](https://github.com/AndunHH/spacemouse) reports as being accepted by 3DxWare and spacenavd. |
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

One RP2040 board and a USB extension cable cut in half — the same build as the
single-Pico [HID Remapper](https://github.com/jfedor2/hid-remapper/blob/master/HARDWARE.md).
The bridge was developed and tested on a **Waveshare RP2040-Zero**; a
Raspberry Pi Pico (or Pico W) builds and is wired the same way but has not
been tried.

| Cable wire (female / device end) | RP2040-Zero pad | Pico pin |
| --- | --- | --- |
| D+ (green) | GP0 | GPIO0 (pin 1) |
| D- (white) | GP1 | GPIO1 (pin 2) |
| VBUS (red) | 5V | VBUS (pin 40) |
| GND (black) | GND | GND (pin 38) |

The board's own USB connector goes to the computer.  See [docs/HARDWARE.md](docs/HARDWARE.md)
for details, the debug UART (GPIO12/13) and power considerations (the
SpacePilot with its backlit LCD is not a low-power device).

## Building

Prerequisites: CMake ≥ 3.13, `gcc-arm-none-eabi` (with newlib), a host C
compiler for the tests.  The Pico SDK and Pico-PIO-USB come in as submodules.

```sh
git clone https://github.com/dewi-ny-je/spacepilot-bridge
cd spacepilot-bridge
git submodule update --init                      # pico-sdk + Pico-PIO-USB
git -C lib/pico-sdk submodule update --init lib/tinyusb

cmake -S . -B build                              # Waveshare RP2040-Zero (default)
cmake -S . -B build -DPICO_BOARD=pico            # or: pico, pico_w
cmake --build build
# -> build/spacepilot_bridge.uf2
```

An existing SDK checkout can be used instead of the submodule by setting
`PICO_SDK_PATH` (and `PICO_PIO_USB_PATH`) in the environment.  The first build
also compiles `picotool` from the SDK to produce the UF2.

Flash it the usual way: hold the BOOT button while plugging the board into the
computer (BOOTSEL on a Pico), then copy `spacepilot_bridge.uf2` onto the
`RPI-RP2` drive that appears.

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

**SpacePilot (046d:c625): examined, compatible** — enumeration, all six axes,
the button table and driver acceptance were all confirmed on real hardware.

**Every other device in the table: not examined.**  Their entries are predicted
from spacenavd's source code rather than observed on hardware, so confirming or
refuting them needs reports from people who own those devices.  If you own one,
this is what to measure, in order, and what to report (an issue with the UART
log and the device name is ideal):

1. **Enumeration on the host port.**  Set `BRIDGE_DEBUG` to 1 (default) and
   watch the UART on GPIO12/13 at 115200 baud.  You should see
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
4. **Driver acceptance.**  The driver under test should list a "SpaceMouse Pro
   Wireless" — this is the actual measurement.

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

No code was copied from those projects: what was taken is protocol knowledge —
USB IDs, report layouts, bit numbers — which is why the reasoning behind every
value is written out in [docs/PROTOCOL.md](docs/PROTOCOL.md) with a pointer to
where it came from.  The SDK and Pico-PIO-USB submodules carry their own
licenses.

This repository's own source is under the Apache License 2.0 (see `LICENSE`).
That licence is permissive, so the personal-use request in
[Scope and intent](#scope-and-intent) is the author's intent and not an extra
licence condition — read that section for why it is worth honouring anyway.

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
