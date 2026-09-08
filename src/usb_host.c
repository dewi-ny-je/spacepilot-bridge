/*
 * usb_host.c - TinyUSB host side: the SpacePilot (Pro) on the PIO USB port.
 */
#include <stdio.h>

#include "pico/time.h"
#include "tusb.h"

#include "bridge.h"
#include "config.h"
#include "usb_host.h"

#if BRIDGE_DEBUG >= 1
#define LOG(...) printf(__VA_ARGS__)
#else
#define LOG(...) do { } while (0)
#endif

static struct {
    bool attached;
    uint8_t dev_addr;
    uint8_t instance;

    bool led_pending;
    bool led_value;
    bool xfer_busy;
    uint8_t led_report[2];

    uint32_t last_activity_ms;
} h;

static inline uint32_t now_ms(void)
{
    return to_ms_since_boot(get_absolute_time());
}

/* The SpacePilot Pro exposes a vendor-specific interface for its LCD and a
 * HID interface for motion + buttons.  TinyUSB only mounts HID interfaces,
 * but other HID devices (keyboard, mouse) could be plugged in by mistake, so
 * require a "Generic Desktop / Multi-axis Controller" top level usage. */
static bool descriptor_is_multiaxis(const uint8_t *desc, uint16_t len)
{
    static const uint8_t magic[] = { 0x05, 0x01, 0x09, 0x08 };
    if (desc == NULL) {
        return false;
    }
    for (uint16_t i = 0; i + sizeof(magic) <= len; i++) {
        if (desc[i] == magic[0] && desc[i + 1] == magic[1] && desc[i + 2] == magic[2] &&
            desc[i + 3] == magic[3]) {
            return true;
        }
    }
    return false;
}

void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance, uint8_t const *desc_report, uint16_t desc_len)
{
    uint16_t vid = 0, pid = 0;
    tuh_vid_pid_get(dev_addr, &vid, &pid);
    LOG("usb_host: HID interface mounted, addr=%u instance=%u vid:pid=%04x:%04x desc_len=%u\n",
        dev_addr, instance, vid, pid, desc_len);

    if (!descriptor_is_multiaxis(desc_report, desc_len)) {
        LOG("usb_host: not a multi-axis controller, ignoring\n");
        return;
    }
    if (h.attached) {
        LOG("usb_host: a source is already attached, ignoring\n");
        return;
    }
    if (!bridge_source_attach(vid, pid)) {
        return;
    }

    h.attached = true;
    h.dev_addr = dev_addr;
    h.instance = instance;
    h.xfer_busy = false;

    if (!tuh_hid_receive_report(dev_addr, instance)) {
        LOG("usb_host: cannot request report\n");
    }
}

void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t instance)
{
    LOG("usb_host: HID interface unmounted, addr=%u instance=%u\n", dev_addr, instance);
    if (h.attached && h.dev_addr == dev_addr && h.instance == instance) {
        h.attached = false;
        h.xfer_busy = false;
        bridge_source_detach();
    }
}

void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance, uint8_t const *report, uint16_t len)
{
    if (h.attached && h.dev_addr == dev_addr && h.instance == instance) {
        h.last_activity_ms = now_ms();
        bridge_source_report(report, len, h.last_activity_ms);
    }
    /* re-arm for the next report */
    tuh_hid_receive_report(dev_addr, instance);
}

void tuh_hid_set_report_complete_cb(uint8_t dev_addr, uint8_t instance, uint8_t report_id,
                                    uint8_t report_type, uint16_t len)
{
    (void) dev_addr;
    (void) instance;
    (void) report_id;
    (void) report_type;
    (void) len;
    h.xfer_busy = false;
}

/* Called by the bridge; the actual transfer is issued from usb_host_task()
 * so that this is safe to call from any context. */
void bridge_hw_set_source_led(bool on)
{
    h.led_value = on;
    h.led_pending = true;
}

void usb_host_task(void)
{
    if (h.attached && h.led_pending && !h.xfer_busy) {
        /* control SET_REPORT: data must start with the report ID (as the
         * Linux kernel and hid-remapper do) */
        h.led_report[0] = BRIDGE_OUT_REPORT_LED;
        h.led_report[1] = h.led_value ? 1 : 0;
        if (tuh_hid_set_report(h.dev_addr, h.instance, BRIDGE_OUT_REPORT_LED, HID_REPORT_TYPE_OUTPUT,
                               h.led_report, sizeof(h.led_report))) {
            h.xfer_busy = true;
            h.led_pending = false;
        }
    }
}

uint32_t usb_host_last_activity_ms(void)
{
    return h.last_activity_ms;
}
