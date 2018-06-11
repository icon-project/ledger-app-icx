#include <os.h>
#include <cx.h>
#include <os_io_seproxyhal.h>
#include "icx_context.h"
#include "glyphs.h"

/*------------------------------------------------------------------------------
    Required global variables from SDK
*/
unsigned char G_io_seproxyhal_spi_buffer[IO_SEPROXYHAL_BUFFER_SIZE_B];
ux_state_t ux;

/*------------------------------------------------------------------------------
    Required global variables for OWN
*/
icx_context_t G_icx_context;


/*------------------------------------------------------------------------------
 * UI Elements & APIs
 * All icon files reside in "glyphs" folder, and it will be generated as
 * "src/glyphs.h" and "src/glyphs.c".
 */
const ux_menu_entry_t menu_main[];
const ux_menu_entry_t menu_about[] = {
    {menu_main, NULL, 0, NULL, "Version", APPVERSION, 0, 0},
    UX_MENU_END
};
const ux_menu_entry_t menu_main[] = {
    {NULL, NULL, 0, NULL, "Start Wallet", "(ICX)", 0, 0},
    {menu_about, NULL, 0, NULL, "About", NULL, 0, 0},
    {NULL, os_sched_exit, 0, &C_nanos_icon_dashboard, "Quit app", NULL, 50, 29},
    UX_MENU_END
};

/**
 * Display IDLE UI.
 * Normally, it should display Main Menu which user can interact.
 */
static void ui_idle(void)
{
    UX_MENU_DISPLAY(0, menu_main, NULL);
}

/*------------------------------------------------------------------------------
    Other APIs
*/
void icx_exit(void) {
    BEGIN_TRY_L(exit) {
        TRY_L(exit) {
            os_sched_exit(-1);
        }
        FINALLY_L(exit) {
        }
    }
    END_TRY_L(exit);
}


/*------------------------------------------------------------------------------
    io_event
*/
void io_seproxyhal_display(const bagl_element_t *element) {
    if ((element->component.type & (~BAGL_TYPE_FLAGS_MASK)) != BAGL_NONE) {
        io_seproxyhal_display_default((bagl_element_t *)element);
    }
}

unsigned short io_exchange_al(unsigned char channel, unsigned short tx_len) {
    switch (channel & ~(IO_FLAGS)) {
    case CHANNEL_KEYBOARD:
        break;
    case CHANNEL_SPI:
        if (tx_len) {
            io_seproxyhal_spi_send(G_io_apdu_buffer, tx_len);

            if (channel & IO_RESET_AFTER_REPLIED) {
                reset();
            }
            return 0; // nothing received from the master so far (it's a tx
                      // transaction)
        } else {
            return io_seproxyhal_spi_recv(G_io_apdu_buffer,
                                          sizeof(G_io_apdu_buffer), 0);
        }
    default:
        THROW(INVALID_PARAMETER);
    }
    return 0;
}

unsigned char io_event(unsigned char channel) {
    switch (G_io_seproxyhal_spi_buffer[0]) {
    case SEPROXYHAL_TAG_BUTTON_PUSH_EVENT:
        UX_BUTTON_PUSH_EVENT(G_io_seproxyhal_spi_buffer);
        break;
    case SEPROXYHAL_TAG_STATUS_EVENT:
        if (G_io_apdu_media == IO_APDU_MEDIA_USB_HID &&
            !(U4BE(G_io_seproxyhal_spi_buffer, 3) &
              SEPROXYHAL_TAG_STATUS_EVENT_FLAG_USB_POWERED)) {
            THROW(EXCEPTION_IO_RESET);
        }
    default:
        UX_DEFAULT_EVENT();
        break;
    case SEPROXYHAL_TAG_DISPLAY_PROCESSED_EVENT:
        UX_DISPLAYED_EVENT({});
        break;
    }

    if (!io_seproxyhal_spi_is_status_sent()) {
        io_seproxyhal_general_status();
    }
    return 1;
}

/**
 * Main application loop.
 * It waits USB communication.
 */
void app_main(void)
{
    os_memset(G_io_apdu_buffer, 0, sizeof(G_io_apdu_buffer));
    os_memset(&G_icx_context, 0, sizeof(G_icx_context));

    for (;;) {
        G_icx_context.in_length = 
            io_exchange(
                CHANNEL_APDU | G_icx_context.io_flags,
                G_icx_context.out_length);
    }
}

/*==============================================================================
    Main function
*/
__attribute__((section(".boot"))) int main(int arg0) {
    __asm volatile("cpsie i");
    os_boot();
    for (;;) {
        UX_INIT();
        BEGIN_TRY {
            TRY {
                io_seproxyhal_init();
                USB_power(0);
                USB_power(1);
                ui_idle();
                app_main();
            }
            CATCH(EXCEPTION_IO_RESET) {
                continue;
            }
            CATCH_ALL {
                break;
            }
            FINALLY {
            }
        } END_TRY;
    }
    icx_exit();
    return 0;
}
