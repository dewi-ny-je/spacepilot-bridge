/*
 * bridge.h - protocol translation core.
 *
 * This module is hardware independent: it consumes raw HID input reports from
 * the source device and produces HID reports for the emulated SpaceMouse Pro
 * Wireless.  The hardware layer (or the unit tests) provides the bridge_hw_*
 * functions declared at the bottom.
 */
#ifndef BRIDGE_H
#define BRIDGE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    BRIDGE_AXIS_X,
    BRIDGE_AXIS_Y,
    BRIDGE_AXIS_Z,
    BRIDGE_AXIS_RX,
    BRIDGE_AXIS_RY,
    BRIDGE_AXIS_RZ,
    BRIDGE_NUM_AXES
};

/* Report IDs of the emulated device (as seen by the computer). */
#define BRIDGE_OUT_REPORT_AXES    1   /* 6 x int16 LE: X Y Z Rx Ry Rz     */
#define BRIDGE_OUT_REPORT_BUTTONS 3   /* 32-bit LE button bitmask          */
#define BRIDGE_OUT_REPORT_LED     4   /* host -> device, bit 0 = LED on    */

#define BRIDGE_AXES_REPORT_LEN    12
#define BRIDGE_BUTTONS_REPORT_LEN 4

/* One entry of a button map: source bit -> destination bit (or SMP_NONE). */
typedef struct {
    uint8_t src;
    uint8_t dst;
} bridge_button_pair_t;

/* Source device quirks. */

/* The device's raw axis convention differs from the emulated SpaceMouse Pro
 * Wireless and has to be normalised: swap Y with Z (and Ry with Rz), then
 * negate Y, Z, Ry and Rz.  This is spacenavd's DF_SWAPYZ | DF_INVYZ, which it
 * applies to every 3Dconnexion device *except* a handful of early Logitech-era
 * ones; since the emulated device gets the transform too, only those few
 * exceptions need it here.  The transform is its own inverse. */
#define BRIDGE_SRC_FIX_YZ 0x01

typedef struct {
    uint16_t vid;
    uint16_t pid;
    const char *name;
    uint8_t flags;
    const bridge_button_pair_t *map;
    uint8_t map_len;
} bridge_source_desc_t;

void bridge_init(void);

/* --- source (USB host) side ---------------------------------------------- */

/* A HID multi-axis device was connected.  Returns false if the bridge does
 * not want it (unknown VID:PID and BRIDGE_ACCEPT_UNKNOWN_SOURCES == 0). */
bool bridge_source_attach(uint16_t vid, uint16_t pid);
void bridge_source_detach(void);
bool bridge_source_attached(void);
const char *bridge_source_name(void);

/* Raw input report from the source, including the leading report ID byte. */
void bridge_source_report(const uint8_t *report, uint16_t len, uint32_t now_ms);

/* --- computer (USB device) side ------------------------------------------ */

/* The computer wrote the LED output report. */
void bridge_host_set_led(bool on);
bool bridge_led_state(void);

/* GET_REPORT from the computer.  Returns the number of bytes written. */
uint16_t bridge_get_report(uint8_t report_id, uint8_t *buf, uint16_t len);

/* Call from the main loop as often as possible. */
void bridge_task(uint32_t now_ms);

/* --- provided by the hardware layer / test harness ------------------------ */

/* Send an input report to the computer.  Return false if the endpoint is busy;
 * the bridge will retry on the next bridge_task(). */
bool bridge_hw_send_report(uint8_t report_id, const uint8_t *data, uint8_t len);

/* Switch the LED of the source device. */
void bridge_hw_set_source_led(bool on);

#ifdef __cplusplus
}
#endif

#endif /* BRIDGE_H */
