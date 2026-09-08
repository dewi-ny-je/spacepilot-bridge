/*
 * buttons.h - symbolic names for button bit numbers used in button_maps.h
 *
 * All values are bit positions inside a little-endian 32-bit bitmask that
 * corresponds to HID report 3 of the respective device.
 */
#ifndef BRIDGE_BUTTONS_H
#define BRIDGE_BUTTONS_H

/* "Not mapped" marker, usable as a destination in a button map entry. */
#define SMP_NONE 0xFF

/*
 * Target: emulated 3Dconnexion SpaceMouse Pro Wireless (report 3, 32 bits).
 *
 * Verified against two independent sources:
 *  - spacenavd, src/dev.c, bnhack_smpro(): evdev BTN_0 + n -> physical key
 *  - AndunHH/spacemouse, config_sample.h: SM_* definitions
 */
#define SMP_MENU   0   /* "Menu"                */
#define SMP_FIT    1   /* "Fit"                 */
#define SMP_T      2   /* "Top" view            */
#define SMP_R      4   /* "Right" view          */
#define SMP_F      5   /* "Front" view          */
#define SMP_RCW    8   /* "Roll 90 degrees CW"  */
#define SMP_1      12  /* "1"                   */
#define SMP_2      13  /* "2"                   */
#define SMP_3      14  /* "3"                   */
#define SMP_4      15  /* "4"                   */
#define SMP_ESC    22  /* "Esc"                 */
#define SMP_ALT    23  /* "Alt"                 */
#define SMP_SHIFT  24  /* "Shift"               */
#define SMP_CTRL   25  /* "Ctrl"                */
#define SMP_ROT    26  /* "Rotation lock"       */

/*
 * Source: 3Dconnexion "V3DK" (virtual 3D key) numbering, bit = key number - 1.
 *
 * This is the layout of report 3 on the SpacePilot Pro (046d:c629): 31 buttons
 * in one contiguous bitfield.  It is also the numbering the SpaceMouse Pro keeps
 * for the keys it still has (compare SMP_* above: the SpaceMouse Pro simply
 * leaves the bits of the missing keys unused), and spacenavd needs no bnhack
 * for the SpacePilot Pro because the bits are contiguous.
 */
#define V3DK_MENU      0
#define V3DK_FIT       1
#define V3DK_TOP       2
#define V3DK_LEFT      3
#define V3DK_RIGHT     4
#define V3DK_FRONT     5
#define V3DK_BOTTOM    6
#define V3DK_BACK      7
#define V3DK_ROLL_CW   8
#define V3DK_ROLL_CCW  9
#define V3DK_ISO1      10
#define V3DK_ISO2      11
#define V3DK_1         12
#define V3DK_2         13
#define V3DK_3         14
#define V3DK_4         15
#define V3DK_5         16
#define V3DK_6         17
#define V3DK_7         18
#define V3DK_8         19
#define V3DK_9         20
#define V3DK_10        21
#define V3DK_ESC       22
#define V3DK_ALT       23
#define V3DK_SHIFT     24
#define V3DK_CTRL      25
#define V3DK_ROTATE    26  /* rotation lock */
#define V3DK_PANZOOM   27
#define V3DK_DOMINANT  28
#define V3DK_PLUS      29
#define V3DK_MINUS     30

/*
 * Source: original SpacePilot (046d:c625), 21 buttons.
 *
 * This layout predates the V3DK numbering.  It is reconstructed from
 * spacenavd's doc/spnavrc_spilot (button 6 = T, 8 = R, 9 = F, 10 = Esc,
 * 11 = Alt, 12 = Shift, 13 = Ctrl, 14 = Fit, 16..18 = sensitivity keys) and
 * from the physical key order of the device.  It has NOT been verified on real
 * hardware: if a key ends up on the wrong function, set BRIDGE_DEBUG to 1,
 * press the key, read the bit number printed on the debug UART and fix the
 * table in button_maps.h.
 */
#define SP_1        0
#define SP_2        1
#define SP_3        2
#define SP_4        3
#define SP_5        4
#define SP_6        5
#define SP_T        6
#define SP_L        7
#define SP_R        8
#define SP_F        9
#define SP_ESC      10
#define SP_ALT      11
#define SP_SHIFT    12
#define SP_CTRL     13
#define SP_FIT      14
#define SP_PANEL    15
#define SP_PLUS     16
#define SP_MINUS    17
#define SP_DOM      18
#define SP_3DLOCK   19
#define SP_CONFIG   20

#endif /* BRIDGE_BUTTONS_H */
