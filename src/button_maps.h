/*
 * button_maps.h - which source key becomes which SpaceMouse Pro key.
 *
 * Edit the tables below to change the mapping.  Each entry is
 *     { <source bit>, <destination bit> }
 * using the names from buttons.h.  Source keys that do not appear in a table
 * are ignored; several source keys may map to the same destination key.
 *
 * This header is included by bridge.c only.
 */
#ifndef BRIDGE_BUTTON_MAPS_H
#define BRIDGE_BUTTON_MAPS_H

#include "bridge.h"
#include "buttons.h"

/*
 * SpacePilot Pro (046d:c629) -> SpaceMouse Pro.
 *
 * Both devices use the V3DK bit numbering, so keys that exist on both map to
 * the same bit.  The SpacePilot Pro keys with no SpaceMouse Pro counterpart
 * (L, B, Back, Roll CCW, ISO1/2, 5..10, Pan/Zoom, Dominant, +, -) are not
 * mapped by default; add lines such as { V3DK_5, SMP_1 } to repurpose them.
 */
static const bridge_button_pair_t map_spacepilot_pro[] = {
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
 * Original SpacePilot (046d:c625) -> SpaceMouse Pro.
 *
 * "Panel" (opens the 3DxWare panel) is mapped to "Menu" (opens the radial
 * menu) and "3D lock" to "Rotation lock" as the closest equivalents.
 * See the note in buttons.h: the SP_* numbering is unverified.
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

#define MAP(m) (m), (sizeof(m) / sizeof((m)[0]))

/* Known source devices.  VID 0x046d is Logitech, which 3Dconnexion used before
 * getting its own 0x256f. */
static const bridge_source_desc_t bridge_sources[] = {
    { 0x046d, 0xc629, "SpacePilot Pro", MAP(map_spacepilot_pro) },
    { 0x046d, 0xc625, "SpacePilot",     MAP(map_spacepilot)     },
};

/* Used for any other multi-axis controller when BRIDGE_ACCEPT_UNKNOWN_SOURCES
 * is set.  All 3Dconnexion devices from the SpacePilot Pro onwards use the
 * V3DK numbering (SpaceExplorer, SpaceMouse Pro, Enterprise, ...). */
static const bridge_source_desc_t bridge_generic_source = {
    0, 0, "generic 6DOF device (V3DK button map)", MAP(map_spacepilot_pro)
};

#undef MAP

#endif /* BRIDGE_BUTTON_MAPS_H */
