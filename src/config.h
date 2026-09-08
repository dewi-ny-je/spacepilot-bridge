/*
 * config.h - compile-time configuration of the SpacePilot bridge.
 *
 * Every setting is wrapped in #ifndef so that it can also be overridden from
 * the command line, e.g.  cmake -DBRIDGE_EXTRA_DEFS="-DBRIDGE_DEBUG=2" ...
 *
 * Button mapping tables live in button_maps.h.
 */
#ifndef BRIDGE_CONFIG_H
#define BRIDGE_CONFIG_H

/* ------------------------------------------------------------------------- */
/* Identity presented to the computer                                        */
/* ------------------------------------------------------------------------- */

/* 3Dconnexion, "SpaceMouse Pro Wireless (cabled)".  0xc632 would be the
 * wireless receiver, 0xc638 the Bluetooth edition. */
#ifndef BRIDGE_USB_VID
#define BRIDGE_USB_VID 0x256f
#endif
#ifndef BRIDGE_USB_PID
#define BRIDGE_USB_PID 0xc631
#endif
#ifndef BRIDGE_USB_BCD_DEVICE
#define BRIDGE_USB_BCD_DEVICE 0x0100
#endif
#ifndef BRIDGE_USB_MANUFACTURER
#define BRIDGE_USB_MANUFACTURER "3Dconnexion"
#endif
#ifndef BRIDGE_USB_PRODUCT
#define BRIDGE_USB_PRODUCT "SpaceMouse Pro Wireless (cabled)"
#endif

/* ------------------------------------------------------------------------- */
/* Source devices                                                            */
/* ------------------------------------------------------------------------- */

/* Accept any HID multi-axis controller whose VID:PID is not listed in
 * button_maps.h, using the generic V3DK button map.  Set to 0 to only accept
 * the devices explicitly listed there. */
#ifndef BRIDGE_ACCEPT_UNKNOWN_SOURCES
#define BRIDGE_ACCEPT_UNKNOWN_SOURCES 1
#endif

/* ------------------------------------------------------------------------- */
/* Report timing                                                             */
/* ------------------------------------------------------------------------- */

/* Minimum spacing between two axis reports sent to the computer.  The real
 * SpaceMouse Pro Wireless (cabled) sends every 8 ms. */
#ifndef BRIDGE_REPORT_INTERVAL_MS
#define BRIDGE_REPORT_INTERVAL_MS 8
#endif

/* Old 3Dconnexion devices send translation (report 1) and rotation (report 2)
 * as two separate USB reports.  The bridge waits up to this long for the
 * second half before sending a combined 6-axis report, so that both halves
 * of one frame end up in the same output report. */
#ifndef BRIDGE_COALESCE_MS
#define BRIDGE_COALESCE_MS 4
#endif

/* Number of all-zero axis reports sent after the cap comes to rest.  The
 * spacemouse project found that 3DxWare wants to see a few of them before it
 * considers the device idle. */
#ifndef BRIDGE_ZERO_REPEAT
#define BRIDGE_ZERO_REPEAT 3
#endif

/* ------------------------------------------------------------------------- */
/* Axis processing                                                           */
/* ------------------------------------------------------------------------- */

/* Order: X, Y, Z, Rx, Ry, Rz.
 *
 * BRIDGE_AXIS_MAP: for each axis *as reported by the source device*, the
 * output axis it is written to (0..5, or 0xFF to drop it).  spacenavd treats
 * the SpacePilot (Pro) and the SpaceMouse Pro Wireless with the same axis
 * flags (DF_SWAPYZ | DF_INVYZ), i.e. both share the same convention, so the
 * default is the identity mapping. */
#ifndef BRIDGE_AXIS_MAP
#define BRIDGE_AXIS_MAP { 0, 1, 2, 3, 4, 5 }
#endif

/* Per *output* axis: 1 to invert the sign. */
#ifndef BRIDGE_AXIS_INVERT
#define BRIDGE_AXIS_INVERT { 0, 0, 0, 0, 0, 0 }
#endif

/* Per *output* axis: gain in percent (100 = pass through). */
#ifndef BRIDGE_AXIS_SCALE_PERCENT
#define BRIDGE_AXIS_SCALE_PERCENT { 100, 100, 100, 100, 100, 100 }
#endif

/* Raw values whose magnitude is below this are treated as zero (0 = off).
 * Sensitivity and dead zones are usually better handled by the driver on the
 * computer; this is only for worn-out sensors. */
#ifndef BRIDGE_AXIS_DEADZONE
#define BRIDGE_AXIS_DEADZONE 0
#endif

/* Output values are clamped to +/- this.  350 is the logical range declared
 * in the SpaceMouse Pro Wireless report descriptor. */
#ifndef BRIDGE_AXIS_CLAMP
#define BRIDGE_AXIS_CLAMP 350
#endif

/* ------------------------------------------------------------------------- */
/* Hardware                                                                  */
/* ------------------------------------------------------------------------- */

/* GPIO of the PIO USB host port D+ line; D- must be on the next GPIO.
 * Matches the single-Pico HID Remapper wiring (GPIO0 = D+, GPIO1 = D-). */
#ifndef BRIDGE_PIO_USB_DP_PIN
#define BRIDGE_PIO_USB_DP_PIN 0
#endif

/* Debug UART (only used when BRIDGE_DEBUG > 0).  GPIO0/1 are taken by the
 * PIO USB host, so uart0 is moved to GPIO16/17. */
#ifndef BRIDGE_UART_TX_PIN
#define BRIDGE_UART_TX_PIN 16
#endif
#ifndef BRIDGE_UART_RX_PIN
#define BRIDGE_UART_RX_PIN 17
#endif
#ifndef BRIDGE_UART_BAUD
#define BRIDGE_UART_BAUD 115200
#endif

/* 0 = silent, 1 = attach/detach and button changes, 2 = every report. */
#ifndef BRIDGE_DEBUG
#define BRIDGE_DEBUG 1
#endif

#endif /* BRIDGE_CONFIG_H */
