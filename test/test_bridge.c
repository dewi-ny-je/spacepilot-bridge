/*
 * test_bridge.c - host-side unit tests for the translation core.
 *
 * Build and run:   make -C test
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bridge.h"
#include "buttons.h"
#include "config.h"

/* ---- fake hardware layer --------------------------------------------------- */

typedef struct {
    uint8_t id;
    uint8_t len;
    uint8_t data[16];
} sent_report_t;

static sent_report_t sent[64];
static int num_sent;
static bool hw_busy;

static int led_calls;
static bool led_last;

bool bridge_hw_send_report(uint8_t report_id, const uint8_t *data, uint8_t len)
{
    if (hw_busy) {
        return false;
    }
    if (num_sent < (int) (sizeof(sent) / sizeof(sent[0]))) {
        sent[num_sent].id = report_id;
        sent[num_sent].len = len;
        memcpy(sent[num_sent].data, data, len);
    }
    num_sent++;
    return true;
}

void bridge_hw_set_source_led(bool on)
{
    led_calls++;
    led_last = on;
}

/* ---- helpers ----------------------------------------------------------------- */

static int failures;

#define CHECK(cond)                                                                   \
    do {                                                                              \
        if (!(cond)) {                                                                \
            printf("  FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);                  \
            failures++;                                                               \
        }                                                                             \
    } while (0)

static void reset(void)
{
    num_sent = 0;
    hw_busy = false;
    led_calls = 0;
    led_last = false;
    bridge_init();
}

static void put16(uint8_t *p, int16_t v)
{
    p[0] = (uint8_t) (v & 0xff);
    p[1] = (uint8_t) ((uint16_t) v >> 8);
}

static int16_t get16(const uint8_t *p)
{
    return (int16_t) ((uint16_t) p[0] | ((uint16_t) p[1] << 8));
}

static void src_trans(int16_t x, int16_t y, int16_t z, uint32_t now)
{
    uint8_t r[7] = { 1 };
    put16(r + 1, x);
    put16(r + 3, y);
    put16(r + 5, z);
    bridge_source_report(r, sizeof(r), now);
}

static void src_rot(int16_t rx, int16_t ry, int16_t rz, uint32_t now)
{
    uint8_t r[7] = { 2 };
    put16(r + 1, rx);
    put16(r + 3, ry);
    put16(r + 5, rz);
    bridge_source_report(r, sizeof(r), now);
}

static void src_buttons(uint32_t mask, int nbytes, uint32_t now)
{
    uint8_t r[5] = { 3 };
    for (int i = 0; i < nbytes; i++) {
        r[1 + i] = (uint8_t) (mask >> (8 * i));
    }
    bridge_source_report(r, (uint16_t) (1 + nbytes), now);
}

static uint32_t buttons_of(const sent_report_t *s)
{
    return (uint32_t) s->data[0] | ((uint32_t) s->data[1] << 8) | ((uint32_t) s->data[2] << 16) |
           ((uint32_t) s->data[3] << 24);
}

/* run the bridge task once per millisecond for `ms` milliseconds */
static void run(uint32_t from, uint32_t ms)
{
    for (uint32_t t = from; t <= from + ms; t++) {
        bridge_task(t);
    }
}

/* attach and drain the initial zero reports */
static uint32_t attach_and_settle(uint16_t vid, uint16_t pid)
{
    CHECK(bridge_source_attach(vid, pid));
    run(0, 100);
    num_sent = 0;
    return 1000;
}

/* ---- tests ---------------------------------------------------------------------- */

static void test_attach(void)
{
    printf("test_attach\n");
    reset();
    CHECK(!bridge_source_attached());
    CHECK(bridge_source_attach(0x046d, 0xc629));
    CHECK(bridge_source_attached());
    CHECK(strcmp(bridge_source_name(), "SpacePilot Pro") == 0);
    CHECK(led_calls == 1); /* LED state re-applied to the new device */
    CHECK(bridge_source_attach(0x046d, 0xc625));
    CHECK(strcmp(bridge_source_name(), "SpacePilot") == 0);
#if BRIDGE_ACCEPT_UNKNOWN_SOURCES
    CHECK(bridge_source_attach(0x1234, 0x5678));
#else
    CHECK(!bridge_source_attach(0x1234, 0x5678));
#endif
    bridge_source_detach();
    CHECK(!bridge_source_attached());
    CHECK(bridge_source_name() == NULL);
}

static void test_initial_zero_frames(void)
{
    printf("test_initial_zero_frames\n");
    reset();
    CHECK(bridge_source_attach(0x046d, 0xc629));
    run(0, 100);
    /* attaching announces a resting device: exactly BRIDGE_ZERO_REPEAT zero reports */
    CHECK(num_sent == BRIDGE_ZERO_REPEAT);
    for (int i = 0; i < num_sent; i++) {
        CHECK(sent[i].id == BRIDGE_OUT_REPORT_AXES);
        CHECK(sent[i].len == BRIDGE_AXES_REPORT_LEN);
        for (int k = 0; k < 6; k++) {
            CHECK(get16(sent[i].data + 2 * k) == 0);
        }
    }
    /* and they respect the report interval */
    run(100, 100);
    CHECK(num_sent == BRIDGE_ZERO_REPEAT);
}

static void test_translation_and_rotation_combined(void)
{
    printf("test_translation_and_rotation_combined\n");
    reset();
    uint32_t t = attach_and_settle(0x046d, 0xc629);

    src_trans(10, -20, 30, t);
    bridge_task(t);
    CHECK(num_sent == 0); /* waiting for the rotation half */
    src_rot(-40, 50, -60, t + 1);
    bridge_task(t + 1);
    CHECK(num_sent == 1);
    CHECK(sent[0].id == BRIDGE_OUT_REPORT_AXES);
    CHECK(sent[0].len == 12);
    CHECK(get16(sent[0].data + 0) == 10);
    CHECK(get16(sent[0].data + 2) == -20);
    CHECK(get16(sent[0].data + 4) == 30);
    CHECK(get16(sent[0].data + 6) == -40);
    CHECK(get16(sent[0].data + 8) == 50);
    CHECK(get16(sent[0].data + 10) == -60);

    /* nothing more without new input */
    run(t + 2, 50);
    CHECK(num_sent == 1);
}

static void test_translation_only_flushes_after_coalesce(void)
{
    printf("test_translation_only_flushes_after_coalesce\n");
    reset();
    uint32_t t = attach_and_settle(0x046d, 0xc629);

    src_trans(100, 0, 0, t);
    for (uint32_t k = 0; k < BRIDGE_COALESCE_MS; k++) {
        bridge_task(t + k);
    }
    CHECK(num_sent == 0);
    bridge_task(t + BRIDGE_COALESCE_MS);
    CHECK(num_sent == 1);
    CHECK(get16(sent[0].data + 0) == 100);
    CHECK(get16(sent[0].data + 6) == 0);
}

static void test_rate_limit(void)
{
    printf("test_rate_limit\n");
    reset();
    uint32_t t = attach_and_settle(0x046d, 0xc629);

    /* the source streams a full frame every millisecond, faster than we send */
    for (uint32_t k = 0; k < 40; k++) {
        src_trans((int16_t) (k + 1), 0, 0, t + k);
        src_rot(0, 0, (int16_t) (k + 1), t + k);
        bridge_task(t + k);
    }
    /* first frame at t, then one every BRIDGE_REPORT_INTERVAL_MS */
    int expected = 1 + (40 - 1) / BRIDGE_REPORT_INTERVAL_MS;
    CHECK(num_sent == expected);
    /* the last one carries the newest values, nothing is queued */
    CHECK(get16(sent[num_sent - 1].data + 0) == get16(sent[num_sent - 1].data + 10));
    CHECK(get16(sent[num_sent - 1].data + 0) > 30);
}

static void test_zero_repeat(void)
{
    printf("test_zero_repeat\n");
    reset();
    uint32_t t = attach_and_settle(0x046d, 0xc629);

    src_trans(5, 5, 5, t);
    src_rot(5, 5, 5, t);
    bridge_task(t);
    CHECK(num_sent == 1);

    /* cap released: the device sends one zero frame and goes quiet */
    src_trans(0, 0, 0, t + 8);
    src_rot(0, 0, 0, t + 8);
    run(t + 8, 200);
    CHECK(num_sent == 1 + BRIDGE_ZERO_REPEAT);
    for (int i = 1; i < num_sent; i++) {
        for (int k = 0; k < 6; k++) {
            CHECK(get16(sent[i].data + 2 * k) == 0);
        }
    }

    /* a source that keeps sending zero frames while idle must not be echoed */
    for (uint32_t k = 0; k < 100; k += 8) {
        src_trans(0, 0, 0, t + 300 + k);
        src_rot(0, 0, 0, t + 300 + k);
        run(t + 300 + k, 7);
    }
    CHECK(num_sent == 1 + BRIDGE_ZERO_REPEAT);
}

static void test_unified_12_byte_report(void)
{
    printf("test_unified_12_byte_report\n");
    reset();
    uint32_t t = attach_and_settle(0x046d, 0xc629);

    uint8_t r[13] = { 1 };
    put16(r + 1, 1);
    put16(r + 3, 2);
    put16(r + 5, 3);
    put16(r + 7, 4);
    put16(r + 9, 5);
    put16(r + 11, 6);
    bridge_source_report(r, sizeof(r), t);
    bridge_task(t);
    CHECK(num_sent == 1); /* complete frame, no coalescing delay */
    for (int k = 0; k < 6; k++) {
        CHECK(get16(sent[0].data + 2 * k) == k + 1);
    }
}

static void test_clamp(void)
{
    printf("test_clamp\n");
    reset();
    uint32_t t = attach_and_settle(0x046d, 0xc629);

    src_trans(32000, -32000, 0, t);
    src_rot(0, 0, 0, t);
    bridge_task(t);
    CHECK(num_sent == 1);
    CHECK(get16(sent[0].data + 0) == BRIDGE_AXIS_CLAMP);
    CHECK(get16(sent[0].data + 2) == -BRIDGE_AXIS_CLAMP);
}

static void test_buttons_spacepilot_pro(void)
{
    printf("test_buttons_spacepilot_pro\n");
    reset();
    uint32_t t = attach_and_settle(0x046d, 0xc629);

    /* Fit + Ctrl + "1" pressed, 4-byte report */
    src_buttons((1u << V3DK_FIT) | (1u << V3DK_CTRL) | (1u << V3DK_1), 4, t);
    bridge_task(t);
    CHECK(num_sent == 1);
    CHECK(sent[0].id == BRIDGE_OUT_REPORT_BUTTONS);
    CHECK(sent[0].len == 4);
    CHECK(buttons_of(&sent[0]) == ((1u << SMP_FIT) | (1u << SMP_CTRL) | (1u << SMP_1)));

    /* an unmapped key (LEFT view) on top: no change on the output */
    src_buttons((1u << V3DK_FIT) | (1u << V3DK_CTRL) | (1u << V3DK_1) | (1u << V3DK_LEFT), 4, t + 1);
    bridge_task(t + 1);
    CHECK(num_sent == 1);

    /* release everything */
    src_buttons(0, 4, t + 2);
    bridge_task(t + 2);
    CHECK(num_sent == 2);
    CHECK(buttons_of(&sent[1]) == 0);

    /* the same state again does not produce a report */
    src_buttons(0, 4, t + 3);
    bridge_task(t + 3);
    CHECK(num_sent == 2);

    /* GET_REPORT reflects the current state */
    src_buttons(1u << V3DK_MENU, 4, t + 4);
    uint8_t buf[8];
    CHECK(bridge_get_report(BRIDGE_OUT_REPORT_BUTTONS, buf, sizeof(buf)) == 4);
    CHECK(buf[0] == (1u << SMP_MENU));
}

static void test_buttons_spacepilot_classic_3_byte(void)
{
    printf("test_buttons_spacepilot_classic_3_byte\n");
    reset();
    uint32_t t = attach_and_settle(0x046d, 0xc625);

    /* Panel -> Menu, 3D lock -> Rotation lock, key 5 -> nothing; 3-byte report */
    src_buttons((1u << SP_PANEL) | (1u << SP_3DLOCK) | (1u << SP_5), 3, t);
    bridge_task(t);
    CHECK(num_sent == 1);
    CHECK(buttons_of(&sent[0]) == ((1u << SMP_MENU) | (1u << SMP_ROT)));
}

static void test_busy_endpoint_retries(void)
{
    printf("test_busy_endpoint_retries\n");
    reset();
    uint32_t t = attach_and_settle(0x046d, 0xc629);

    hw_busy = true;
    src_buttons(1u << V3DK_ESC, 4, t);
    src_trans(7, 0, 0, t);
    src_rot(0, 0, 7, t);
    run(t, 20);
    CHECK(num_sent == 0);
    hw_busy = false;
    run(t + 21, 20);
    CHECK(num_sent == 2);
    CHECK(sent[0].id == BRIDGE_OUT_REPORT_BUTTONS);
    CHECK(buttons_of(&sent[0]) == (1u << SMP_ESC));
    CHECK(sent[1].id == BRIDGE_OUT_REPORT_AXES);
    CHECK(get16(sent[1].data + 0) == 7);
    CHECK(get16(sent[1].data + 10) == 7);
}

static void test_detach_releases_everything(void)
{
    printf("test_detach_releases_everything\n");
    reset();
    uint32_t t = attach_and_settle(0x046d, 0xc629);

    src_buttons(1u << V3DK_SHIFT, 4, t);
    src_trans(50, 50, 50, t);
    src_rot(50, 50, 50, t);
    bridge_task(t);
    CHECK(num_sent == 2);
    num_sent = 0;

    bridge_source_detach();
    run(t + 10, 200);
    CHECK(num_sent == 1 + BRIDGE_ZERO_REPEAT);
    CHECK(sent[0].id == BRIDGE_OUT_REPORT_BUTTONS);
    CHECK(buttons_of(&sent[0]) == 0);
    for (int i = 1; i < num_sent; i++) {
        CHECK(sent[i].id == BRIDGE_OUT_REPORT_AXES);
        for (int k = 0; k < 6; k++) {
            CHECK(get16(sent[i].data + 2 * k) == 0);
        }
    }

    /* reports from a detached device are ignored */
    src_trans(1, 1, 1, t + 300);
    src_rot(1, 1, 1, t + 300);
    run(t + 300, 50);
    CHECK(num_sent == 1 + BRIDGE_ZERO_REPEAT);
}

static void test_led(void)
{
    printf("test_led\n");
    reset();
    uint8_t buf[4];

    CHECK(!bridge_led_state());
    bridge_host_set_led(true);
    CHECK(bridge_led_state());
    CHECK(led_calls == 1 && led_last == true);
    CHECK(bridge_get_report(BRIDGE_OUT_REPORT_LED, buf, sizeof(buf)) == 1);
    CHECK(buf[0] == 1);

    /* a device attached later gets the current LED state */
    CHECK(bridge_source_attach(0x046d, 0xc629));
    CHECK(led_calls == 2 && led_last == true);

    bridge_host_set_led(false);
    CHECK(led_calls == 3 && led_last == false);
    CHECK(bridge_get_report(BRIDGE_OUT_REPORT_LED, buf, sizeof(buf)) == 1);
    CHECK(buf[0] == 0);

    /* unknown / too small requests */
    CHECK(bridge_get_report(0x17, buf, sizeof(buf)) == 0);
    CHECK(bridge_get_report(BRIDGE_OUT_REPORT_AXES, buf, sizeof(buf)) == 0);
}

static void test_short_and_unknown_reports_are_ignored(void)
{
    printf("test_short_and_unknown_reports_are_ignored\n");
    reset();
    uint32_t t = attach_and_settle(0x046d, 0xc629);

    uint8_t r1[3] = { 1, 0x10, 0x00 };
    uint8_t r2[4] = { 2, 0x10, 0x00, 0x10 };
    uint8_t r5[5] = { 5, 1, 2, 3, 4 };
    bridge_source_report(r1, sizeof(r1), t);
    bridge_source_report(r2, sizeof(r2), t);
    bridge_source_report(r5, sizeof(r5), t);
    bridge_source_report(r5, 0, t);
    run(t, 50);
    CHECK(num_sent == 0);
}

static void test_every_spacenavd_usb_device_attaches(void)
{
    printf("test_every_spacenavd_usb_device_attaches\n");
    static const uint16_t ids[][2] = {
        { 0x046d, 0xc603 }, { 0x046d, 0xc605 }, { 0x046d, 0xc606 }, { 0x046d, 0xc621 },
        { 0x046d, 0xc623 }, { 0x046d, 0xc625 }, { 0x046d, 0xc626 }, { 0x046d, 0xc627 },
        { 0x046d, 0xc628 }, { 0x046d, 0xc629 }, { 0x046d, 0xc62b }, { 0x046d, 0xc640 },
        { 0x256f, 0xc62e }, { 0x256f, 0xc62f }, { 0x256f, 0xc631 }, { 0x256f, 0xc632 },
        { 0x256f, 0xc633 }, { 0x256f, 0xc635 }, { 0x256f, 0xc636 }, { 0x256f, 0xc638 },
        { 0x256f, 0xc63a },
    };
    reset();
    for (size_t i = 0; i < sizeof(ids) / sizeof(ids[0]); i++) {
        CHECK(bridge_source_attach(ids[i][0], ids[i][1]));
        CHECK(bridge_source_name() != NULL);
        CHECK(strncmp(bridge_source_name(), "unknown", 7) != 0); /* found in the table */
    }
}

static void test_fix_yz_axes(void)
{
    printf("test_fix_yz_axes\n");
    reset();
    /* SpaceMouse Classic: one of the three devices spacenavd leaves un-normalised */
    uint32_t t = attach_and_settle(0x046d, 0xc606);

    src_trans(1, 2, 3, t);
    src_rot(4, 5, 6, t);
    bridge_task(t);
    CHECK(num_sent == 1);
    /* swap Y<->Z, Ry<->Rz, then negate Y, Z, Ry, Rz */
    CHECK(get16(sent[0].data + 0) == 1);
    CHECK(get16(sent[0].data + 2) == -3);
    CHECK(get16(sent[0].data + 4) == -2);
    CHECK(get16(sent[0].data + 6) == 4);
    CHECK(get16(sent[0].data + 8) == -6);
    CHECK(get16(sent[0].data + 10) == -5);

    /* a device without the flag passes straight through */
    reset();
    t = attach_and_settle(0x046d, 0xc626);
    src_trans(1, 2, 3, t);
    src_rot(4, 5, 6, t);
    bridge_task(t);
    CHECK(num_sent == 1);
    for (int k = 0; k < 6; k++) {
        CHECK(get16(sent[0].data + 2 * k) == k + 1);
    }
}

static void test_sequential_map(void)
{
    printf("test_sequential_map\n");
    reset();
    /* SpaceNavigator: two keys, bits 0 and 1, which are Menu and Fit */
    uint32_t t = attach_and_settle(0x046d, 0xc626);
    src_buttons(0x3, 1, t);
    bridge_task(t);
    CHECK(num_sent == 1);
    CHECK(buttons_of(&sent[0]) == ((1u << SMP_MENU) | (1u << SMP_FIT)));

    /* SpaceExplorer: 15 contiguous keys; the 15th lands on the 15th
     * SpaceMouse Pro key, a 16th would have nowhere to go */
    reset();
    t = attach_and_settle(0x046d, 0xc627);
    src_buttons((1u << 14) | (1u << 15), 2, t);
    bridge_task(t);
    CHECK(num_sent == 1);
    CHECK(buttons_of(&sent[0]) == (1u << SMP_ROT));
}

static void test_enterprise_wide_button_report(void)
{
    printf("test_enterprise_wide_button_report\n");
    reset();
    uint32_t t = attach_and_settle(0x256f, 0xc633);

    /* 22-byte report 3: "Space" is bit 175 (unmapped by default) */
    uint8_t r[23] = { 3 };
    r[1 + 175 / 8] = (uint8_t) (1u << (175 % 8));
    bridge_source_report(r, sizeof(r), t);
    bridge_task(t);
    CHECK(num_sent == 0);

    /* add "Menu" (bit 0): only that one reaches the computer */
    r[1] = 1u << V3DK_MENU;
    bridge_source_report(r, sizeof(r), t + 1);
    bridge_task(t + 1);
    CHECK(num_sent == 1);
    CHECK(buttons_of(&sent[0]) == (1u << SMP_MENU));

    /* a report longer than the bitmask we keep is truncated, not misread */
    uint8_t big[64] = { 3 };
    big[1] = 1u << V3DK_FIT;
    big[63] = 0xff;
    bridge_source_report(big, sizeof(big), t + 2);
    bridge_task(t + 2);
    CHECK(num_sent == 2);
    CHECK(buttons_of(&sent[1]) == (1u << SMP_FIT));
}

int main(void)
{
    test_attach();
    test_every_spacenavd_usb_device_attaches();
    test_fix_yz_axes();
    test_sequential_map();
    test_enterprise_wide_button_report();
    test_initial_zero_frames();
    test_translation_and_rotation_combined();
    test_translation_only_flushes_after_coalesce();
    test_rate_limit();
    test_zero_repeat();
    test_unified_12_byte_report();
    test_clamp();
    test_buttons_spacepilot_pro();
    test_buttons_spacepilot_classic_3_byte();
    test_busy_endpoint_retries();
    test_detach_releases_everything();
    test_led();
    test_short_and_unknown_reports_are_ignored();

    if (failures) {
        printf("%d check(s) FAILED\n", failures);
        return EXIT_FAILURE;
    }
    printf("all tests passed\n");
    return EXIT_SUCCESS;
}
