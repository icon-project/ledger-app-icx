#include <os.h>
#include <cx.h>
#include <os_io_seproxyhal.h>
#include "icx_context.h"

#define MAX_BIP32_PATH 10

#define CLA 0xE0
#define INS_GET_PUBLIC_KEY 0x02
#define INS_SIGN 0x04
#define INS_GET_APP_CONFIGURATION 0x06

#define P1_CONFIRM 0x01
#define P1_NON_CONFIRM 0x00
#define P2_NO_CHAINCODE 0x00
#define P2_CHAINCODE 0x01
#define P1_FIRST 0x00
#define P1_MORE 0x80

#define OFFSET_CLA 0
#define OFFSET_INS 1
#define OFFSET_P1 2
#define OFFSET_P2 3
#define OFFSET_LC 4
#define OFFSET_CDATA 5

//#include "glyphs.h"

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
    {NULL, os_sched_exit, 0, NULL, "Quit app", NULL, 0, 0},
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

#if 0
void handleGetPublicKey(uint8_t p1, uint8_t p2, uint8_t *dataBuffer,
                        uint16_t dataLength, volatile unsigned int *flags,
                        volatile unsigned int *tx) {
    UNUSED(dataLength);
    uint8_t privateKeyData[32];
    uint32_t bip32Path[MAX_BIP32_PATH];
    uint32_t i;
    uint8_t bip32PathLength = *(dataBuffer++);
    cx_ecfp_private_key_t privateKey;

    if ((bip32PathLength < 0x01) || (bip32PathLength > MAX_BIP32_PATH)) {
        PRINTF("Invalid path\n");
        THROW(0x6a80);
    }
    if ((p1 != P1_CONFIRM) && (p1 != P1_NON_CONFIRM)) {
        THROW(0x6B00);
    }
    if ((p2 != P2_CHAINCODE) && (p2 != P2_NO_CHAINCODE)) {
        THROW(0x6B00);
    }
    for (i = 0; i < bip32PathLength; i++) {
        bip32Path[i] = (dataBuffer[0] << 24) | (dataBuffer[1] << 16) |
                       (dataBuffer[2] << 8) | (dataBuffer[3]);
        dataBuffer += 4;
    }
    tmpCtx.publicKeyContext.getChaincode = (p2 == P2_CHAINCODE);
    os_perso_derive_node_bip32(CX_CURVE_256K1, bip32Path, bip32PathLength,
                               privateKeyData,
                               (tmpCtx.publicKeyContext.getChaincode
                                    ? tmpCtx.publicKeyContext.chainCode
                                    : NULL));
    cx_ecfp_init_private_key(CX_CURVE_256K1, privateKeyData, 32, &privateKey);
    cx_ecfp_generate_pair(CX_CURVE_256K1, &tmpCtx.publicKeyContext.publicKey,
                          &privateKey, 1);
    os_memset(&privateKey, 0, sizeof(privateKey));
    os_memset(privateKeyData, 0, sizeof(privateKeyData));
    getEthAddressStringFromKey(&tmpCtx.publicKeyContext.publicKey,
                               tmpCtx.publicKeyContext.address, &sha3);
    if (p1 == P1_NON_CONFIRM) {
        *tx = set_result_get_publicKey();
        THROW(0x9000);
    } else {
        /*
        addressSummary[0] = '0';
        addressSummary[1] = 'x';
        os_memmove((unsigned char *)(addressSummary + 2),
        tmpCtx.publicKeyContext.address, 4);
        os_memmove((unsigned char *)(addressSummary + 6), "...", 3);
        os_memmove((unsigned char *)(addressSummary + 9),
        tmpCtx.publicKeyContext.address + 40 - 4, 4);
        addressSummary[13] = '\0';
        */

        // prepare for a UI based reply
        skipWarning = false;
#if defined(TARGET_BLUE)
        snprintf(fullAddress, sizeof(fullAddress), "0x%.*s", 40,
                 tmpCtx.publicKeyContext.address);
        UX_DISPLAY(ui_address_blue, ui_address_blue_prepro);
#elif defined(TARGET_NANOS)
        snprintf(fullAddress, sizeof(fullAddress), "0x%.*s", 40,
                 tmpCtx.publicKeyContext.address);
        ux_step = 0;
        ux_step_count = 2;
        UX_DISPLAY(ui_address_nanos, ui_address_prepro);
#endif // #if TARGET_ID

        *flags |= IO_ASYNCH_REPLY;
    }
}

void handleSign(uint8_t p1, uint8_t p2, uint8_t *workBuffer,
                uint16_t dataLength, volatile unsigned int *flags,
                volatile unsigned int *tx) {
}
#endif

void handleGetAppConfiguration(uint8_t p1, uint8_t p2, uint8_t *workBuffer,
                               uint16_t dataLength,
                               volatile unsigned int *flags,
                               volatile unsigned int *tx) {
    UNUSED(p1);
    UNUSED(p2);
    UNUSED(workBuffer);
    UNUSED(dataLength);
    UNUSED(flags);
    G_io_apdu_buffer[0] = LEDGER_MAJOR_VERSION;
    G_io_apdu_buffer[1] = LEDGER_MINOR_VERSION;
    G_io_apdu_buffer[2] = LEDGER_PATCH_VERSION;
    *tx = 3;
    THROW(0x9000);
}

void handleApdu(volatile unsigned int *flags, volatile unsigned int *tx) {
    unsigned short sw = 0;

    BEGIN_TRY {
        TRY {
            if (G_io_apdu_buffer[OFFSET_CLA] != CLA) {
                THROW(0x6E00);
            }

            switch (G_io_apdu_buffer[OFFSET_INS]) {
            case INS_GET_PUBLIC_KEY:
                /*
                handleGetPublicKey(G_io_apdu_buffer[OFFSET_P1],
                                   G_io_apdu_buffer[OFFSET_P2],
                                   G_io_apdu_buffer + OFFSET_CDATA,
                                   G_io_apdu_buffer[OFFSET_LC], flags, tx);
                 */
                break;

            case INS_SIGN:
                /*
                handleSign(G_io_apdu_buffer[OFFSET_P1],
                           G_io_apdu_buffer[OFFSET_P2],
                           G_io_apdu_buffer + OFFSET_CDATA,
                           G_io_apdu_buffer[OFFSET_LC], flags, tx);
                 */
                break;

            case INS_GET_APP_CONFIGURATION:
                handleGetAppConfiguration(
                    G_io_apdu_buffer[OFFSET_P1], G_io_apdu_buffer[OFFSET_P2],
                    G_io_apdu_buffer + OFFSET_CDATA,
                    G_io_apdu_buffer[OFFSET_LC], flags, tx);
                break;

            default:
                THROW(0x6D00);
                break;
            }
        }
        CATCH(EXCEPTION_IO_RESET) {
            THROW(EXCEPTION_IO_RESET);
        }
        CATCH_OTHER(e) {
            switch (e & 0xF000) {
            case 0x6000:
                sw = e;
                break;
            case 0x9000:
                // OK
                sw = e;
                break;
            default:
                // Internal error
                sw = 0x6800 | (e & 0x7FF);
                break;
            }
            // Unexpected exception => report
            G_io_apdu_buffer[*tx] = sw >> 8;
            G_io_apdu_buffer[*tx + 1] = sw;
            *tx += 2;
        }
        FINALLY {
        }
    }
    END_TRY;
}

/**
 * Main application loop.
 * It waits USB communication.
 */
void app_main(void)
{
    volatile unsigned int rx = 0;
    volatile unsigned int tx = 0;
    volatile unsigned int flags = 0;

    for (;;) {
        volatile unsigned short sw = 0;

        BEGIN_TRY {
            TRY {
                rx = tx;
                tx = 0; // ensure no race in catch_other if io_exchange throws
                        // an error
                rx = io_exchange(CHANNEL_APDU | flags, rx);
                flags = 0;

                // no apdu received, well, reset the session, and reset the
                // bootloader configuration
                if (rx == 0) {
                    THROW(0x6982);
                }

                handleApdu(&flags, &tx);
            }
            CATCH(EXCEPTION_IO_RESET) {
                THROW(EXCEPTION_IO_RESET);
            }
            CATCH_OTHER(e) {
                switch (e & 0xF000) {
                case 0x6000:
                    // Wipe the transaction context and report the exception
                    sw = e;
                    break;
                case 0x9000:
                    // All is well
                    sw = e;
                    break;
                default:
                    // Internal error
                    sw = 0x6800 | (e & 0x7FF);
                    break;
                }
                // Unexpected exception => report
                G_io_apdu_buffer[tx] = sw >> 8;
                G_io_apdu_buffer[tx + 1] = sw;
                tx += 2;
            }
            FINALLY {
            }
        }
        END_TRY;
    }

    // return_to_dashboard:
    return;
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
