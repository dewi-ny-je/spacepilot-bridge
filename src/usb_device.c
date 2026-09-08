/*
 * usb_device.c - TinyUSB device side: the emulated SpaceMouse Pro Wireless
 * on the RP2040's native USB port.
 */
#include "tusb.h"

#include "bridge.h"
#include "usb_descriptors.h"

uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance)
{
    (void) instance;
    return desc_hid_report;
}

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type,
                               uint8_t *buffer, uint16_t reqlen)
{
    (void) instance;
    (void) report_type;
    return bridge_get_report(report_id, buffer, reqlen);
}

/* Called both for a control SET_REPORT and for data on the interrupt OUT
 * endpoint.  In the first case TinyUSB passes the report ID separately (and
 * strips it from the buffer), in the second case report_id is 0 and the
 * buffer starts with the report ID byte. */
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type,
                           uint8_t const *buffer, uint16_t bufsize)
{
    (void) instance;
    (void) report_type;

    if (report_id == 0 && bufsize >= 2) {
        report_id = buffer[0];
        buffer++;
        bufsize--;
    }

    if (report_id == BRIDGE_OUT_REPORT_LED && bufsize >= 1) {
        bridge_host_set_led((buffer[0] & 0x01) != 0);
    }
}

bool tud_hid_set_idle_cb(uint8_t instance, uint8_t idle_rate)
{
    (void) instance;
    (void) idle_rate;
    return true; /* accept; we never send reports on an idle timer anyway */
}

bool bridge_hw_send_report(uint8_t report_id, const uint8_t *data, uint8_t len)
{
    if (!tud_hid_ready()) {
        return false;
    }
    return tud_hid_report(report_id, data, len);
}
