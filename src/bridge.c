/*
 * bridge.c - protocol translation core (hardware independent).
 *
 * Source protocol (SpacePilot, SpacePilot Pro and other pre-2011 3Dconnexion
 * devices), as the kernel/spacenavd see it:
 *   report 1: X, Y, Z        as 3 x int16 LE (6 bytes)
 *   report 2: Rx, Ry, Rz     as 3 x int16 LE (6 bytes)
 *   report 3: button bitmask, 3 or 4 bytes LE
 *   report 4: LED, 1 byte (output)
 * Some newer firmware puts all six axes into report 1 (12 bytes); that is
 * handled too.
 *
 * Target protocol (SpaceMouse Pro Wireless, see usb_descriptors.c):
 *   report 1: X, Y, Z, Rx, Ry, Rz as 6 x int16 LE (12 bytes)
 *   report 3: 32-bit LE button bitmask (4 bytes)
 *   report 4: LED, 1 byte (output)
 */
#include <stdio.h>
#include <string.h>

#include "bridge.h"
#include "button_maps.h"
#include "config.h"

#if BRIDGE_DEBUG >= 1
#define LOG(...) printf(__VA_ARGS__)
#else
#define LOG(...) do { } while (0)
#endif
#if BRIDGE_DEBUG >= 2
#define LOG2(...) printf(__VA_ARGS__)
#else
#define LOG2(...) do { } while (0)
#endif

#define SRC_REPORT_TRANSLATION 1
#define SRC_REPORT_ROTATION    2
#define SRC_REPORT_BUTTONS     3

static const uint8_t axis_map[BRIDGE_NUM_AXES] = BRIDGE_AXIS_MAP;
static const uint8_t axis_invert[BRIDGE_NUM_AXES] = BRIDGE_AXIS_INVERT;
static const int16_t axis_scale[BRIDGE_NUM_AXES] = BRIDGE_AXIS_SCALE_PERCENT;

static struct {
    const bridge_source_desc_t *src;
    uint8_t btn_lut[32];          /* source bit -> destination bit / SMP_NONE */

    int16_t axes[BRIDGE_NUM_AXES];
    bool axes_dirty;
    bool got_trans, got_rot;      /* halves received since the last flush */
    uint32_t last_rx_ms;
    uint32_t last_axes_sent_ms;
    uint8_t zeros_sent;           /* consecutive all-zero reports sent */

    uint32_t src_buttons_raw;
    uint32_t buttons;             /* mapped, current */
    uint32_t buttons_sent;        /* mapped, last delivered to the computer */

    bool led;
} st;

/* ------------------------------------------------------------------------- */

static const bridge_source_desc_t *find_source(uint16_t vid, uint16_t pid)
{
    for (size_t i = 0; i < sizeof(bridge_sources) / sizeof(bridge_sources[0]); i++) {
        if (bridge_sources[i].vid == vid && bridge_sources[i].pid == pid) {
            return &bridge_sources[i];
        }
    }
#if BRIDGE_ACCEPT_UNKNOWN_SOURCES
    return &bridge_generic_source;
#else
    return NULL;
#endif
}

static void build_button_lut(const bridge_source_desc_t *d)
{
    memset(st.btn_lut, SMP_NONE, sizeof(st.btn_lut));
    for (size_t i = 0; i < d->map_len; i++) {
        if (d->map[i].src < 32 && d->map[i].dst < 32) {
            st.btn_lut[d->map[i].src] = d->map[i].dst;
        }
    }
}

static bool axes_nonzero(void)
{
    for (int i = 0; i < BRIDGE_NUM_AXES; i++) {
        if (st.axes[i] != 0) {
            return true;
        }
    }
    return false;
}

static void reset_motion_state(void)
{
    memset(st.axes, 0, sizeof(st.axes));
    st.axes_dirty = true;
    /* both halves "present" so the zero frame is flushed without waiting */
    st.got_trans = st.got_rot = true;
    st.zeros_sent = 0;
    st.src_buttons_raw = 0;
    st.buttons = 0;
}

/* ------------------------------------------------------------------------- */

void bridge_init(void)
{
    memset(&st, 0, sizeof(st));
    memset(st.btn_lut, SMP_NONE, sizeof(st.btn_lut));
}

bool bridge_source_attach(uint16_t vid, uint16_t pid)
{
    const bridge_source_desc_t *d = find_source(vid, pid);
    if (d == NULL) {
        LOG("bridge: ignoring unknown source %04x:%04x\n", vid, pid);
        return false;
    }
    st.src = d;
    build_button_lut(d);
    reset_motion_state();
    LOG("bridge: source attached: %s [%04x:%04x]\n", d->name, vid, pid);
    /* make the new device show the LED state the computer last asked for */
    bridge_hw_set_source_led(st.led);
    return true;
}

void bridge_source_detach(void)
{
    if (st.src != NULL) {
        LOG("bridge: source detached: %s\n", st.src->name);
    }
    st.src = NULL;
    /* release everything on the computer side */
    reset_motion_state();
}

bool bridge_source_attached(void)
{
    return st.src != NULL;
}

const char *bridge_source_name(void)
{
    return st.src ? st.src->name : NULL;
}

/* ------------------------------------------------------------------------- */

static int16_t process_axis(uint8_t axis, int16_t raw)
{
    int32_t v = raw;
#if BRIDGE_AXIS_DEADZONE > 0
    if (v > -BRIDGE_AXIS_DEADZONE && v < BRIDGE_AXIS_DEADZONE) {
        v = 0;
    }
#endif
    v = (v * axis_scale[axis]) / 100;
    if (axis_invert[axis]) {
        v = -v;
    }
    if (v > BRIDGE_AXIS_CLAMP) {
        v = BRIDGE_AXIS_CLAMP;
    } else if (v < -BRIDGE_AXIS_CLAMP) {
        v = -BRIDGE_AXIS_CLAMP;
    }
    return (int16_t) v;
}

/* p points at `count` int16 LE values for source axes first..first+count-1 */
static void update_axes(const uint8_t *p, int first, int count)
{
    bool changed = false;

    for (int i = 0; i < count; i++) {
        int src_axis = first + i;
        int16_t raw = (int16_t) ((uint16_t) p[2 * i] | ((uint16_t) p[2 * i + 1] << 8));
        uint8_t dst = axis_map[src_axis];
        if (dst >= BRIDGE_NUM_AXES) {
            continue; /* axis dropped by configuration */
        }
        int16_t v = process_axis(dst, raw);
        if (v != st.axes[dst]) {
            st.axes[dst] = v;
            changed = true;
        }
    }

    /* A deflected cap must keep producing reports (the driver integrates them),
     * a resting one only until BRIDGE_ZERO_REPEAT zero reports went out. */
    if (changed || axes_nonzero() || st.zeros_sent < BRIDGE_ZERO_REPEAT) {
        st.axes_dirty = true;
    }
}

static void update_buttons(const uint8_t *p, uint16_t n)
{
    uint32_t raw = 0;
    if (n > 4) {
        n = 4;
    }
    for (uint16_t i = 0; i < n; i++) {
        raw |= (uint32_t) p[i] << (8 * i);
    }

    if (raw != st.src_buttons_raw) {
#if BRIDGE_DEBUG >= 1
        printf("bridge: source buttons 0x%08lx:", (unsigned long) raw);
        for (int bit = 0; bit < 32; bit++) {
            if (raw & (1u << bit)) {
                printf(" %d%s", bit, st.btn_lut[bit] == SMP_NONE ? "(unmapped)" : "");
            }
        }
        printf("\n");
#endif
        st.src_buttons_raw = raw;
    }

    uint32_t mapped = 0;
    for (int bit = 0; bit < 32; bit++) {
        if ((raw & (1u << bit)) && st.btn_lut[bit] != SMP_NONE) {
            mapped |= 1u << st.btn_lut[bit];
        }
    }
    st.buttons = mapped;
}

void bridge_source_report(const uint8_t *report, uint16_t len, uint32_t now_ms)
{
    if (st.src == NULL || len < 1) {
        return;
    }
    const uint8_t id = report[0];
    const uint8_t *p = report + 1;
    const uint16_t n = len - 1;

#if BRIDGE_DEBUG >= 2
    printf("bridge: rx id=%u len=%u:", id, n);
    for (uint16_t i = 0; i < n; i++) {
        printf(" %02x", p[i]);
    }
    printf("\n");
#endif

    switch (id) {
    case SRC_REPORT_TRANSLATION:
        if (n >= 12) {
            /* unified report with all six axes */
            update_axes(p, 0, 6);
            st.got_trans = st.got_rot = true;
        } else if (n >= 6) {
            update_axes(p, 0, 3);
            st.got_trans = true;
        } else {
            return;
        }
        st.last_rx_ms = now_ms;
        break;

    case SRC_REPORT_ROTATION:
        if (n >= 6) {
            update_axes(p, 3, 3);
            st.got_rot = true;
            st.last_rx_ms = now_ms;
        }
        break;

    case SRC_REPORT_BUTTONS:
        update_buttons(p, n);
        break;

    default:
        LOG2("bridge: ignoring report id %u\n", id);
        break;
    }
}

/* ------------------------------------------------------------------------- */

void bridge_host_set_led(bool on)
{
    if (on != st.led) {
        LOG("bridge: LED %s\n", on ? "on" : "off");
    }
    st.led = on;
    bridge_hw_set_source_led(on);
}

bool bridge_led_state(void)
{
    return st.led;
}

static void pack_axes(uint8_t *buf)
{
    for (int i = 0; i < BRIDGE_NUM_AXES; i++) {
        buf[2 * i] = (uint8_t) (st.axes[i] & 0xff);
        buf[2 * i + 1] = (uint8_t) ((uint16_t) st.axes[i] >> 8);
    }
}

static void pack_buttons(uint8_t *buf, uint32_t buttons)
{
    for (int i = 0; i < BRIDGE_BUTTONS_REPORT_LEN; i++) {
        buf[i] = (uint8_t) (buttons >> (8 * i));
    }
}

uint16_t bridge_get_report(uint8_t report_id, uint8_t *buf, uint16_t len)
{
    switch (report_id) {
    case BRIDGE_OUT_REPORT_AXES:
        if (len < BRIDGE_AXES_REPORT_LEN) {
            return 0;
        }
        pack_axes(buf);
        return BRIDGE_AXES_REPORT_LEN;
    case BRIDGE_OUT_REPORT_BUTTONS:
        if (len < BRIDGE_BUTTONS_REPORT_LEN) {
            return 0;
        }
        pack_buttons(buf, st.buttons);
        return BRIDGE_BUTTONS_REPORT_LEN;
    case BRIDGE_OUT_REPORT_LED:
        if (len < 1) {
            return 0;
        }
        buf[0] = st.led ? 1 : 0;
        return 1;
    default:
        return 0;
    }
}

/* ------------------------------------------------------------------------- */

static void send_axes(uint32_t now_ms)
{
    uint8_t buf[BRIDGE_AXES_REPORT_LEN];
    pack_axes(buf);
    if (!bridge_hw_send_report(BRIDGE_OUT_REPORT_AXES, buf, sizeof(buf))) {
        return; /* endpoint busy, retry next time */
    }
    st.last_axes_sent_ms = now_ms;
    st.axes_dirty = false;
    st.got_trans = st.got_rot = false;
    if (axes_nonzero()) {
        st.zeros_sent = 0;
    } else if (st.zeros_sent < 255) {
        st.zeros_sent++;
    }
}

static void send_buttons(void)
{
    uint8_t buf[BRIDGE_BUTTONS_REPORT_LEN];
    pack_buttons(buf, st.buttons);
    if (bridge_hw_send_report(BRIDGE_OUT_REPORT_BUTTONS, buf, sizeof(buf))) {
        LOG("bridge: sent buttons 0x%08lx\n", (unsigned long) st.buttons);
        st.buttons_sent = st.buttons;
    }
}

void bridge_task(uint32_t now_ms)
{
    if (st.buttons != st.buttons_sent) {
        send_buttons();
    }

    if ((uint32_t) (now_ms - st.last_axes_sent_ms) < BRIDGE_REPORT_INTERVAL_MS) {
        return;
    }

    if (st.axes_dirty) {
        bool frame_complete = st.got_trans && st.got_rot;
        bool waited = (uint32_t) (now_ms - st.last_rx_ms) >= BRIDGE_COALESCE_MS;
        if (frame_complete || waited) {
            send_axes(now_ms);
        }
    } else if (st.zeros_sent > 0 && st.zeros_sent < BRIDGE_ZERO_REPEAT) {
        send_axes(now_ms);
    }
}
