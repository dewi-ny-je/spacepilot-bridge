/*
 * main.c - SpacePilot bridge firmware for the Raspberry Pi Pico (RP2040).
 *
 *   SpacePilot (Pro) --USB--> [PIO USB host, GPIO0/1] RP2040 [native USB] --> computer
 *                                                      sees a SpaceMouse Pro Wireless
 */
#include <stdio.h>

#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "pico/stdlib.h"
#include "pico/stdio_uart.h"
#include "pio_usb.h"
#include "tusb.h"

#include "bridge.h"
#include "config.h"
#include "usb_host.h"

static inline uint32_t now_ms(void)
{
    return to_ms_since_boot(get_absolute_time());
}

/* Status LED (Pico only; the Pico W LED hangs off the WiFi chip):
 *   fast blink  - not enumerated by the computer
 *   slow blink  - waiting for a source device
 *   solid       - bridging; flickers while reports are flowing */
static void status_led_init(void)
{
#ifdef PICO_DEFAULT_LED_PIN
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
#endif
}

static void status_led_task(uint32_t now)
{
#ifdef PICO_DEFAULT_LED_PIN
    bool on;
    if (!tud_mounted()) {
        on = (now / 100) & 1;
    } else if (!bridge_source_attached()) {
        on = (now / 500) & 1;
    } else {
        on = (uint32_t) (now - usb_host_last_activity_ms()) > 40;
    }
    gpio_put(PICO_DEFAULT_LED_PIN, on);
#else
    (void) now;
#endif
}

int main(void)
{
    /* Pico-PIO-USB needs a system clock that is a multiple of 12 MHz */
    set_sys_clock_khz(120000, true);

#if BRIDGE_DEBUG > 0
    stdio_uart_init_full(uart0, BRIDGE_UART_BAUD, BRIDGE_UART_TX_PIN, BRIDGE_UART_RX_PIN);
    printf("\nspacepilot-bridge starting\n");
#endif

    status_led_init();
    bridge_init();

    /* host on the PIO port */
    pio_usb_configuration_t pio_cfg = PIO_USB_DEFAULT_CONFIG;
    pio_cfg.pin_dp = BRIDGE_PIO_USB_DP_PIN;
    tuh_configure(BOARD_TUH_RHPORT, TUH_CFGID_RPI_PIO_USB_CONFIGURATION, &pio_cfg);

    tusb_rhport_init_t dev_init = {
        .role = TUSB_ROLE_DEVICE,
        .speed = TUSB_SPEED_FULL,
    };
    tusb_init(BOARD_TUD_RHPORT, &dev_init);

    tusb_rhport_init_t host_init = {
        .role = TUSB_ROLE_HOST,
        .speed = TUSB_SPEED_FULL,
    };
    tusb_init(BOARD_TUH_RHPORT, &host_init);

    while (true) {
        tud_task();
        tuh_task();
        usb_host_task();

        uint32_t now = now_ms();
        bridge_task(now);
        status_led_task(now);
    }

    return 0;
}
