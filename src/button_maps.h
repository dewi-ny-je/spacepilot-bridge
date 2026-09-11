/*
 * button_maps.h - which source key becomes which SpaceMouse Pro key, and which
 * source devices are recognised at all.
 *
 * Edit the tables below to change a mapping.  Each entry is
 *     { <source bit>, <destination bit> }
 * using the names from buttons.h.  Source keys that do not appear in a table
 * are ignored; several source keys may map to the same destination key.
 *
 * The emulated SpaceMouse Pro has 15 keys, so any source device with more of
 * them will have keys left over.  Map them onto something useful by adding a
 * line; the debug UART prints the bit number of every key you press.
 *
 * This header is included by bridge.c only.
 */
#ifndef BRIDGE_BUTTON_MAPS_H
#define BRIDGE_BUTTON_MAPS_H

#include "bridge.h"
#include "buttons.h"

/*
 * Map A: devices that number their keys with 3Dconnexion's V3DK codes
 * (bit = key number - 1).  Keys that exist on both the source and the
 * SpaceMouse Pro keep the same bit, so this is the identity.
 *
 * Applies to every device for which spacenavd needs a button remapping hack
 * (bnhack_smpro / bnhack_sment in its src/dev.c) - the hack exists precisely
 * because those devices leave gaps where a key is missing - plus the
 * SpacePilot Pro, which has all 31 keys and so needs no hack despite using the
 * same numbering.
 */
static const bridge_button_pair_t map_v3dk[] = {
    { V3DK_MENU,    SMP_MENU  },
    { V3DK_FIT,     SMP_FIT   },
    { V3DK_TOP,     SMP_T     },
    { V3DK_RIGHT,   SMP_R     },
    { V3DK_FRONT,   SMP_F     },
    { V3DK_ROLL_CW, SMP_RCW   },
    { V3DK_1,       SMP_1     },
    { V3DK_2,       SMP_2     },
    { V3DK_3,       SMP_3     },
    { V3DK_4,       SMP_4     },
    { V3DK_ESC,     SMP_ESC   },
    { V3DK_ALT,     SMP_ALT   },
    { V3DK_SHIFT,   SMP_SHIFT },
    { V3DK_CTRL,    SMP_CTRL  },
    { V3DK_ROTATE,  SMP_ROT   },
};

/*
 * Map B: devices that simply number their keys 0, 1, 2, ... in the order they
 * are laid out, which is every device for which spacenavd needs no remapping
 * hack.  Their key order is not the V3DK order, so an identity map would drop
 * keys into the SpaceMouse Pro's unused bits; instead the n-th source key is
 * given the n-th SpaceMouse Pro key.
 *
 * For the two-button devices (SpaceNavigator, SpaceMouse Compact, SpaceMouse
 * Wireless) this is exactly right: their keys are "Menu" and "Fit".  For the
 * larger ones it is a best-effort default, since the physical key order is not
 * documented in any of the reference projects - check it with BRIDGE_DEBUG=1
 * and reorder the list if a key ends up in the wrong place.
 */
static const bridge_button_pair_t map_sequential[] = {
    { 0,  SMP_MENU  },
    { 1,  SMP_FIT   },
    { 2,  SMP_T     },
    { 3,  SMP_R     },
    { 4,  SMP_F     },
    { 5,  SMP_RCW   },
    { 6,  SMP_1     },
    { 7,  SMP_2     },
    { 8,  SMP_3     },
    { 9,  SMP_4     },
    { 10, SMP_ESC   },
    { 11, SMP_ALT   },
    { 12, SMP_SHIFT },
    { 13, SMP_CTRL  },
    { 14, SMP_ROT   },
};

/*
 * Map C: original SpacePilot (046d:c625).
 *
 * Reconstructed from spacenavd's doc/spnavrc_spilot (button 6 = T, 8 = R,
 * 9 = F, 10 = Esc, 11 = Alt, 12 = Shift, 13 = Ctrl, 14 = Fit, 16..18 =
 * sensitivity keys) rather than guessed, so it gets its own table.  "Panel"
 * (opens the 3DxWare panel) goes to "Menu" (opens the radial menu) and
 * "3D lock" to "Rotation lock" as the closest equivalents.  See the note in
 * buttons.h: the numbering is still unverified on hardware.
 */
static const bridge_button_pair_t map_spacepilot[] = {
    { SP_1,      SMP_1     },
    { SP_2,      SMP_2     },
    { SP_3,      SMP_3     },
    { SP_4,      SMP_4     },
    { SP_T,      SMP_T     },
    { SP_R,      SMP_R     },
    { SP_F,      SMP_F     },
    { SP_ESC,    SMP_ESC   },
    { SP_ALT,    SMP_ALT   },
    { SP_SHIFT,  SMP_SHIFT },
    { SP_CTRL,   SMP_CTRL  },
    { SP_FIT,    SMP_FIT   },
    { SP_PANEL,  SMP_MENU  },
    { SP_3DLOCK, SMP_ROT   },
};

#define MAP(m) (m), (uint8_t) (sizeof(m) / sizeof((m)[0]))

/*
 * Every USB device in spacenavd's table (src/dev.c), minus the ones it
 * blacklists as not being 6DOF controllers (CadMouse, Keyboard Pro, ...) -
 * those are rejected anyway because their report descriptor does not declare a
 * multi-axis controller.  spacenavd's serial-only devices (Spaceball 1003 /
 * 2003 / 3003 / 4000, Magellan SpaceMouse, serial CadMan) are not here: they
 * are RS-232 devices, not USB, and would need different hardware.
 *
 * VID 0x046d is Logitech, which 3Dconnexion used before getting 0x256f.
 *
 * BRIDGE_SRC_FIX_YZ is set for exactly those devices that spacenavd does *not*
 * give DF_SWAPYZ | DF_INVYZ: it adds those flags to every 0x256f device
 * unconditionally, and lists them for all the 0x046d ones except the three
 * below, whose axes therefore need normalising here.
 */
static const bridge_source_desc_t bridge_sources[] = {
    /* --- Logitech-era devices ------------------------------------------- */
    { 0x046d, 0xc603, "SpaceMouse Plus XT",          BRIDGE_SRC_FIX_YZ, MAP(map_sequential) },
    { 0x046d, 0xc605, "CadMan",                      0,                 MAP(map_sequential) },
    { 0x046d, 0xc606, "SpaceMouse Classic",          BRIDGE_SRC_FIX_YZ, MAP(map_sequential) },
    { 0x046d, 0xc621, "Spaceball 5000",              0,                 MAP(map_sequential) },
    { 0x046d, 0xc623, "Space Traveller",             0,                 MAP(map_sequential) },
    { 0x046d, 0xc625, "SpacePilot",                  0,                 MAP(map_spacepilot) },
    { 0x046d, 0xc626, "SpaceNavigator",              0,                 MAP(map_sequential) },
    { 0x046d, 0xc627, "SpaceExplorer",               0,                 MAP(map_sequential) },
    { 0x046d, 0xc628, "SpaceNavigator for Notebooks", 0,                MAP(map_sequential) },
    { 0x046d, 0xc629, "SpacePilot Pro",              0,                 MAP(map_v3dk)       },
    { 0x046d, 0xc62b, "SpaceMouse Pro",              0,                 MAP(map_v3dk)       },
    { 0x046d, 0xc640, "NuLOOQ",                      BRIDGE_SRC_FIX_YZ, MAP(map_sequential) },

    /* --- 3Dconnexion-era devices ---------------------------------------- */
    { 0x256f, 0xc62e, "SpaceMouse Wireless (cabled)", 0,                MAP(map_sequential) },
    { 0x256f, 0xc62f, "SpaceMouse Wireless receiver", 0,                MAP(map_sequential) },
    { 0x256f, 0xc631, "SpaceMouse Pro Wireless",      0,                MAP(map_v3dk)       },
    { 0x256f, 0xc632, "SpaceMouse Pro Wireless receiver", 0,            MAP(map_v3dk)       },
    { 0x256f, 0xc633, "SpaceMouse Enterprise",        0,                MAP(map_v3dk)       },
    { 0x256f, 0xc635, "SpaceMouse Compact",           0,                MAP(map_sequential) },
    { 0x256f, 0xc636, "SpaceMouse Module",            0,                MAP(map_sequential) },
    { 0x256f, 0xc638, "SpaceMouse Pro Wireless BT",   0,                MAP(map_v3dk)       },
    { 0x256f, 0xc63a, "SpaceMouse Wireless (BT)",     0,                MAP(map_sequential) },
};

/* Used for any other multi-axis controller when BRIDGE_ACCEPT_UNKNOWN_SOURCES
 * is set.  V3DK is the better guess for anything newer than this table. */
static const bridge_source_desc_t bridge_generic_source = {
    0, 0, "unknown 6DOF device (V3DK button map)", 0, MAP(map_v3dk)
};

#undef MAP

#endif /* BRIDGE_BUTTON_MAPS_H */
