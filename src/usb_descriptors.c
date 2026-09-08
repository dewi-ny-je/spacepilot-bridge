/*
 * usb_descriptors.c - the emulated SpaceMouse Pro Wireless (cabled).
 *
 * The HID report descriptor is the reverse-engineered one used by the
 * AndunHH/spacemouse project (SpaceMouseHID.h), which is known to be accepted
 * by 3DxWare on Windows and by spacenavd on Linux.  It matches the Wireshark
 * capture of a real SpaceMouse Wireless for reports 1, 3 and 4.
 */
#include <string.h>

#include "pico/unique_id.h"
#include "tusb.h"

#include "config.h"
#include "usb_descriptors.h"

/* ------------------------------------------------------------------------- */
/* HID report descriptor                                                     */
/* ------------------------------------------------------------------------- */

const uint8_t desc_hid_report[] = {
    0x05, 0x01,        /* Usage Page (Generic Desktop)                  */
    0x09, 0x08,        /* Usage (Multi-axis Controller)                 */
    0xA1, 0x01,        /* Collection (Application)                      */

    /* Report 1: six axes */
    0xA1, 0x00,        /*   Collection (Physical)                       */
    0x85, 0x01,        /*     Report ID (1)                             */
    0x16, 0xA2, 0xFE,  /*     Logical Minimum (-350)                    */
    0x26, 0x5E, 0x01,  /*     Logical Maximum (350)                     */
    0x36, 0x88, 0xFA,  /*     Physical Minimum (-1400)                  */
    0x46, 0x78, 0x05,  /*     Physical Maximum (1400)                   */
    0x55, 0x0C,        /*     Unit Exponent (-4)                        */
    0x65, 0x11,        /*     Unit (SI Linear, Centimeter)              */
    0x09, 0x30,        /*     Usage (X)                                 */
    0x09, 0x31,        /*     Usage (Y)                                 */
    0x09, 0x32,        /*     Usage (Z)                                 */
    0x09, 0x33,        /*     Usage (Rx)                                */
    0x09, 0x34,        /*     Usage (Ry)                                */
    0x09, 0x35,        /*     Usage (Rz)                                */
    0x75, 0x10,        /*     Report Size (16)                          */
    0x95, 0x06,        /*     Report Count (6)                          */
    0x81, 0x02,        /*     Input (Data,Var,Abs)                      */
    0xC0,              /*   End Collection                              */

    /* Report 3: buttons */
    0xA1, 0x00,        /*   Collection (Physical)                       */
    0x85, 0x03,        /*     Report ID (3)                             */
    0x15, 0x00,        /*     Logical Minimum (0)                       */
    0x25, 0x01,        /*     Logical Maximum (1)                       */
    0x75, 0x01,        /*     Report Size (1)                           */
    0x95, 0x20,        /*     Report Count (32)                         */
    0x05, 0x09,        /*     Usage Page (Button)                       */
    0x19, 0x01,        /*     Usage Minimum (Button 1)                  */
    0x29, 0x20,        /*     Usage Maximum (Button 32)                 */
    0x81, 0x02,        /*     Input (Data,Var,Abs)                      */
    0xC0,              /*   End Collection                              */

    /* Report 4: LED (output) */
    0xA1, 0x02,        /*   Collection (Logical)                        */
    0x85, 0x04,        /*     Report ID (4)                             */
    0x05, 0x08,        /*     Usage Page (LEDs)                         */
    0x09, 0x4B,        /*     Usage (Generic Indicator)                 */
    0x15, 0x00,        /*     Logical Minimum (0)                       */
    0x25, 0x01,        /*     Logical Maximum (1)                       */
    0x95, 0x01,        /*     Report Count (1)                          */
    0x75, 0x01,        /*     Report Size (1)                           */
    0x91, 0x02,        /*     Output (Data,Var,Abs)                     */
    0x95, 0x01,        /*     Report Count (1)                          */
    0x75, 0x07,        /*     Report Size (7)                           */
    0x91, 0x03,        /*     Output (Const,Var,Abs)  - padding         */
    0xC0,              /*   End Collection                              */

    0xC0               /* End Collection                                */
};

const uint16_t desc_hid_report_len = sizeof(desc_hid_report);

/* ------------------------------------------------------------------------- */
/* Device descriptor                                                         */
/* ------------------------------------------------------------------------- */

static const tusb_desc_device_t desc_device = {
    .bLength = sizeof(tusb_desc_device_t),
    .bDescriptorType = TUSB_DESC_DEVICE,
    .bcdUSB = 0x0200,
    .bDeviceClass = 0x00,
    .bDeviceSubClass = 0x00,
    .bDeviceProtocol = 0x00,
    .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,

    .idVendor = BRIDGE_USB_VID,
    .idProduct = BRIDGE_USB_PID,
    .bcdDevice = BRIDGE_USB_BCD_DEVICE,

    .iManufacturer = 0x01,
    .iProduct = 0x02,
    .iSerialNumber = 0x03,

    .bNumConfigurations = 0x01,
};

uint8_t const *tud_descriptor_device_cb(void)
{
    return (uint8_t const *) &desc_device;
}

/* ------------------------------------------------------------------------- */
/* Configuration descriptor                                                  */
/* ------------------------------------------------------------------------- */

#define EPNUM_HID_OUT 0x02
#define EPNUM_HID_IN  0x81

#define CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_HID_INOUT_DESC_LEN)

static const uint8_t desc_configuration[] = {
    /* config number, interface count, string index, total length, attribute, power in mA */
    TUD_CONFIG_DESCRIPTOR(1, 1, 0, CONFIG_TOTAL_LEN, 0, 100),

    /* interface number, string index, protocol, report descriptor len,
     * EP out & in address, size & polling interval */
    TUD_HID_INOUT_DESCRIPTOR(0, 0, HID_ITF_PROTOCOL_NONE, sizeof(desc_hid_report),
                             EPNUM_HID_OUT, EPNUM_HID_IN, CFG_TUD_HID_EP_BUFSIZE, 1),
};

uint8_t const *tud_descriptor_configuration_cb(uint8_t index)
{
    (void) index;
    return desc_configuration;
}

/* ------------------------------------------------------------------------- */
/* String descriptors                                                        */
/* ------------------------------------------------------------------------- */

enum {
    STRID_LANGID = 0,
    STRID_MANUFACTURER,
    STRID_PRODUCT,
    STRID_SERIAL,
};

static uint16_t _desc_str[32 + 1];

uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid)
{
    (void) langid;
    size_t chr_count;

    switch (index) {
    case STRID_LANGID:
        _desc_str[1] = 0x0409; /* English (US) */
        chr_count = 1;
        break;

    case STRID_SERIAL: {
        char serial[2 * PICO_UNIQUE_BOARD_ID_SIZE_BYTES + 1];
        pico_get_unique_board_id_string(serial, sizeof(serial));
        chr_count = strlen(serial);
        for (size_t i = 0; i < chr_count; i++) {
            _desc_str[1 + i] = (uint16_t) serial[i];
        }
        break;
    }

    case STRID_MANUFACTURER:
    case STRID_PRODUCT: {
        const char *str = (index == STRID_MANUFACTURER) ? BRIDGE_USB_MANUFACTURER : BRIDGE_USB_PRODUCT;
        chr_count = strlen(str);
        if (chr_count > 32) {
            chr_count = 32;
        }
        for (size_t i = 0; i < chr_count; i++) {
            _desc_str[1 + i] = (uint16_t) str[i];
        }
        break;
    }

    default:
        return NULL;
    }

    /* first byte is length (including header), second byte is string type */
    _desc_str[0] = (uint16_t) ((TUSB_DESC_STRING << 8) | (2 * chr_count + 2));
    return _desc_str;
}
