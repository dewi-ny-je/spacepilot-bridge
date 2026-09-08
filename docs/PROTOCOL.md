# Protocol notes

What the bridge translates between, and where each piece of information comes
from.  All multi-byte values are little-endian.

## Source: SpacePilot Pro (046d:c629) and SpacePilot (046d:c625)

USB vendor 0x046d is Logitech; 3Dconnexion used it until they got 0x256f.  The
IDs and the axis flags below come from `src/dev.c` in
[spacenavd](https://github.com/FreeSpacenav/spacenavd):

```
{{0x046d, 0xc625}, DEV_SPILOT,    DF_SWAPYZ | DF_INVYZ, 0},   /* space pilot */
{{0x046d, 0xc629}, DEV_SPILOTPRO, DF_SWAPYZ | DF_INVYZ, 0},   /* space pilot pro */
{{0x256f, 0xc631}, DEV_SMPROW,    DF_SWAPYZ | DF_INVYZ, bnhack_smpro}, /* spacemouse pro wireless */
```

The identical `DF_*` flags on both sides mean the raw axis order and sign of the
SpacePilot (Pro) already match the SpaceMouse Pro Wireless, so the bridge maps
axes 1:1 (`BRIDGE_AXIS_MAP` is the identity).

### Interfaces

The SpacePilot Pro has two USB interfaces:

* interface 0 — vendor specific (class 0xFF): the colour LCD and its bezel keys
  (see [3dxdisp-pro](https://github.com/MiguelDLM/3dxdisp-pro)); the bridge
  ignores it (TinyUSB does not even mount it).
* interface 1 — HID, usage page Generic Desktop / usage Multi-axis Controller:
  motion and the 31 keys.  This is what the bridge attaches to.

The original SpacePilot has a monochrome LCD driven by HID feature reports (see
[3dxdisp](https://github.com/jtsiomb/3dxdisp)); the bridge never sends feature
reports, so the LCD is left alone.

### Input reports (device → bridge)

The classic 3Dconnexion HID layout, shared by every 3Dconnexion USB device up
to the SpaceMouse Pro (046d:c62b):

| Report ID | Length | Content |
| --- | --- | --- |
| 1 | 6 | X, Y, Z as `int16` (translation) |
| 2 | 6 | Rx, Ry, Rz as `int16` (rotation) |
| 3 | 3 or 4 | button bitmask, bit *n* = button *n* |

Values are nominally in the −350…350 range.  Translation and rotation arrive as
two separate reports; some firmware revisions send all six axes in a 12-byte
report 1 instead, and the bridge accepts that too.

Buttons: the SpacePilot Pro uses 3Dconnexion's *V3DK* numbering with bit =
key − 1 (Menu = 0, Fit = 1, Top = 2, Left = 3, Right = 4, Front = 5, Bottom = 6,
Back = 7, Roll CW = 8, Roll CCW = 9, ISO1 = 10, ISO2 = 11, keys 1–10 = 12–21,
Esc = 22, Alt = 23, Shift = 24, Ctrl = 25, Rotate = 26, Pan/Zoom = 27,
Dominant = 28, + = 29, − = 30).  Two facts support this: spacenavd needs no
button remapping hack for the SpacePilot Pro (the bits are contiguous), and the
SpaceMouse Pro keeps exactly these bit positions for the keys it retains
(`bnhack_smpro()` in spacenavd: bit 0 Menu, 1 Fit, 2 T, 4 R, 5 F, 8 roll,
12–15 keys 1–4, 22–26 Esc/Alt/Shift/Ctrl/Rotate).

The original SpacePilot predates V3DK.  Its layout in `src/buttons.h` is
reconstructed from spacenavd's `doc/spnavrc_spilot` (6 = T, 8 = R, 9 = F,
10 = Esc, 11 = Alt, 12 = Shift, 13 = Ctrl, 14 = Fit, 16–18 = +/−/Dom used as
sensitivity keys) and is unverified.

### Output report (bridge → device)

| Report ID | Length | Content |
| --- | --- | --- |
| 4 | 1 | bit 0: LED on |

Sent as a control `SET_REPORT(Output, 4)` whose data starts with the report
ID byte — the same framing the Linux kernel and HID Remapper use.

## Target: SpaceMouse Pro Wireless (cabled), 256f:c631

Report descriptor and behaviour from [AndunHH/spacemouse](https://github.com/AndunHH/spacemouse)
(`SpaceMouseHID.h/.cpp`, `set_hwids.py`), which is a proven emulation of this
device, cross-checked against the Wireshark capture of a SpaceMouse Wireless in
that repository's `SpaceMouseWireless.md`.

| Report ID | Dir | Length | Content |
| --- | --- | --- | --- |
| 1 | in | 12 | X, Y, Z, Rx, Ry, Rz as `int16`, logical range −350…350 |
| 3 | in | 4 | 32-bit button bitmask (bit numbers as above, `SMP_*` in `buttons.h`) |
| 4 | out | 1 | bit 0: LED on |

Behaviour reproduced by the bridge:

* Axis reports are sent at most every `BRIDGE_REPORT_INTERVAL_MS` (8 ms) while
  the cap is deflected; the driver integrates them.
* When the cap comes to rest, `BRIDGE_ZERO_REPEAT` (3) all-zero reports are
  sent, then nothing.  3DxWare needs to see a few zero frames to stop moving.
* A button report is sent only when the mapped button state changes.

Not reproduced: the battery report (ID 0x17) of the wireless variants and the
vendor-defined collection the real device also exposes.  The AndunHH project
gets by without them, using the same VID/PID.

## The bridge

```
                 report 1 (XYZ)  ─┐
 SpacePilot ───  report 2 (RxRyRz)├─▶ merge, wait ≤ BRIDGE_COALESCE_MS for the
                 report 3 (keys) ─┤   other half, rate-limit ─▶ report 1 (6 axes)
                                  └─▶ map bits through button_maps.h ─▶ report 3
 SpacePilot ◀──  report 4 (LED)  ◀──  forwarded from the computer's report 4
```

`src/bridge.c` holds the whole translation and has no TinyUSB dependency;
`test/test_bridge.c` exercises it on the host.
