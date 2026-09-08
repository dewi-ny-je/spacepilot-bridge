# Hardware

## Parts

* Raspberry Pi Pico (RP2040).  A Pico W works too, but its LED is not used.
* A USB extension cable (A male to A female), cut in half.  The **female** half
  is what the SpacePilot plugs into.
* Optional: a USB-to-serial adapter for the debug output.

## Wiring

The RP2040's own USB port talks to the computer.  A second, software USB host
port is created with [Pico-PIO-USB](https://github.com/sekigon-gonnoc/Pico-PIO-USB)
on two GPIOs.  This is the single-Pico [HID Remapper](https://github.com/jfedor2/hid-remapper/blob/master/HARDWARE.md)
build; its photos apply 1:1.

| USB wire (female half) | Colour (usually) | Pico pin |
| --- | --- | --- |
| D+ | green | GPIO0 (pin 1) |
| D- | white | GPIO1 (pin 2) |
| VBUS | red | VBUS (pin 40) |
| GND | black | GND (pin 38) |

Keep the D+/D- wires short (a few centimetres) and of similar length.  Series
resistors are not needed.

The D+ GPIO can be changed with `BRIDGE_PIO_USB_DP_PIN` in `src/config.h`;
D- is always the next GPIO.

## Power

VBUS on pin 40 is the 5 V coming from the computer, passed straight through to
the SpacePilot.  A SpacePilot Pro with its colour LCD and backlight draws
noticeably more than a plain SpaceMouse; the bridge declares 100 mA in its
configuration descriptor (like the real SpaceMouse Pro Wireless) but the actual
draw is the sum of the Pico (~30 mA) and the SpacePilot.  A normal USB port
tolerates this in practice; if the SpacePilot does not enumerate or resets
under load, feed VBUS from a powered hub or a separate 5 V supply (common GND,
and do not back-feed the computer).

## Debug UART

With `BRIDGE_DEBUG` ≥ 1 (the default) the firmware prints to `uart0` on
**GPIO16 (TX)** / **GPIO17 (RX)** at 115200 baud — the default GPIO0/1 are taken
by the PIO USB port.  Connect a 3.3 V serial adapter (adapter RX to GPIO16,
GND to GND).

```
spacepilot-bridge starting
usb_host: HID interface mounted, addr=1 instance=0 vid:pid=046d:c629 desc_len=...
bridge: source attached: SpacePilot Pro [046d:c629]
bridge: LED on
bridge: source buttons 0x00000002: 1
bridge: sent buttons 0x00000002
```

`BRIDGE_DEBUG=2` additionally prints every raw report received from the source
(this slows things down; use it only to inspect the protocol).

## Status LED (Pico only)

| Pattern | Meaning |
| --- | --- |
| fast blink (10 Hz) | not enumerated by the computer |
| slow blink (1 Hz) | enumerated, waiting for a SpacePilot on the host port |
| solid | bridging; flickers while reports flow |

## Notes on the RP2040 build

* The system clock runs at 120 MHz (a multiple of the 12 MHz USB bit rate that
  Pico-PIO-USB needs).
* The binary is copied to RAM at boot (`copy_to_ram`) so the bit-banged USB
  host is never stalled by XIP flash accesses.  It is about 130 KB.
* Everything runs on core 0: the PIO USB host does its low-level work from a
  1 ms timer interrupt, the TinyUSB device/host tasks and the bridge run in the
  main loop.
