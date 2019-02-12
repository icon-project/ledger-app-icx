/*******************************************************************************
*   Ledger Blue
*   (c) 2016 Ledger
*
*   Modifications (c) 2018 icon foundation
*
*  Licensed under the Apache License, Version 2.0 (the "License");
*  you may not use this file except in compliance with the License.
*  You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
*  Unless required by applicable law or agreed to in writing, software
*  distributed under the License is distributed on an "AS IS" BASIS,
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*  See the License for the specific language governing permissions and
*  limitations under the License.
********************************************************************************/

#include <os.h>
#include <cx.h>
#include <os_io_seproxyhal.h>
#include "uint256.h"

#define TEST_MODE   0

#define MAX_BIP32_PATH 10
#define MAX_VALUE_LEN 66

#define CLA 0xE0
#define INS_GET_PUBLIC_KEY 0x02
#define INS_SIGN 0x04
#define INS_GET_APP_CONFIGURATION 0x06
#define INS_SET_TEST_PRIVATE_KEY 0xFF

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

enum {
    SW_OK = 0x9000,
    SW_BAD_LENGTH = 0x6700,
    SW_BAD_STATE = 0x6000,
    SW_BAD_PARAM = 0x6B00,
    SW_BAD_DATA = 0x6A80,
    SW_BAD_CLA = 0x6E00,
    SW_BAD_INST = 0x6D00,
    SW_USER_DENIAL = 0x6985,
    SW_NO_APDU = 0x6982,
    SW_INTERNAL_ERROR_MASK = 0x6F00,
};

//  APDU IO
typedef struct APDUIO {
    uint32_t flags;
    uint16_t rx;
    uint16_t tx;
} APDUIO;

APDUIO g_aio;

#define g_aio_buf G_io_apdu_buffer

inline void aio_write8(uint8_t v) {
    g_aio_buf[g_aio.tx++] = v & 0xFF;
}

inline void aio_write16(uint16_t sw) {
    g_aio_buf[g_aio.tx++] = (sw>>8) & 0xFF;
    g_aio_buf[g_aio.tx++] = sw & 0xFF;
}

inline void aio_write(const uint8_t* in, int len) {
    os_memmove(g_aio_buf+g_aio.tx, in, len);
    g_aio.tx += len;
}

/*------------------------------------------------------------------------------
    Required global variables from SDK
*/
unsigned char G_io_seproxyhal_spi_buffer[IO_SEPROXYHAL_BUFFER_SIZE_B];
ux_state_t ux;

unsigned int ux_step;
unsigned int ux_step_count;

typedef struct publicKeyContext_t {
    cx_ecfp_public_key_t publicKey;
    uint8_t address[43];
    uint8_t chainCode[32];
    bool getChaincode;
} publicKeyContext_t;

enum {
    PARSER_BUF_SIZE = 128,
    ICX_EXP = 18,
    TEST_BIP32 = 0x80000000,
    PARSER_STATE_KEY_FLAG = 0x100,
    PARSER_STATE_VALUE_FLAG = 0x200,
};

typedef enum {
    PARSER_STATE_PREFIX,
    PARSER_STATE_KEY            = PARSER_STATE_KEY_FLAG,
    PARSER_STATE_KEY_IGNORE,
    PARSER_STATE_VALUE_FEE      = PARSER_STATE_VALUE_FLAG,
    PARSER_STATE_VALUE_STEP_LIMIT,
    PARSER_STATE_VALUE_TO,
    PARSER_STATE_VALUE_VALUE,
    PARSER_STATE_VALUE_VERSION,
    PARSER_STATE_VALUE_IGNORE
} PARSER_STATE;

typedef struct {
    PARSER_STATE state;
    int nestingLevel;
    bool isEscaping;
    char* wp;           //  write position
    char buf[PARSER_BUF_SIZE];

    bool hasTo;
    uint8_t to[43];

    bool hasValue;
    uint256_t value;

    bool hasFee;
    uint256_t fee;

    bool hasStepLimit;
    uint256_t stepLimit;

    bool hasVersion;
    uint32_t version;
} Parser;

typedef struct transactionContext_t {
    uint8_t pathLength;
    uint32_t bip32Path[MAX_BIP32_PATH];
    uint8_t hash[32];
    uint32_t txLeft;
    Parser parser;
} transactionContext_t;

union {
    publicKeyContext_t publicKeyContext;
    transactionContext_t transactionContext;
} tmpCtx;

cx_sha3_t sha3;

#if TEST_MODE
cx_ecfp_private_key_t testPrivateKey;
#endif

bool g_isSigning;
char fullAddress[44];
char fullAmount[50];
char feeName[10];
char fullFee[50];
bool skipWarning;

/*------------------------------------------------------------------------------
 * UI Elements & APIs
 * All icon files reside in "glyphs" folder, and it will be generated as
 * "src/glyphs.h" and "src/glyphs.c".
 */
void set_result_get_publicKey() {
    aio_write8(65);
    aio_write(tmpCtx.publicKeyContext.publicKey.W, 65);
    aio_write8(42);
    aio_write(tmpCtx.publicKeyContext.address, 42);
    if (tmpCtx.publicKeyContext.getChaincode)
        aio_write(tmpCtx.publicKeyContext.chainCode, 32);
}

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

const bagl_element_t ui_address_nanos[] = {
    // type                               userid    x    y   w    h  str rad
    // fill      fg        bg      fid iid  txt   touchparams...       ]
    {{BAGL_RECTANGLE, 0x00, 0, 0, 128, 32, 0, 0, BAGL_FILL, 0x000000, 0xFFFFFF,
      0, 0},
     NULL,
     0,
     0,
     0,
     NULL,
     NULL,
     NULL},

    {{BAGL_ICON, 0x00, 3, 12, 7, 7, 0, 0, 0, 0xFFFFFF, 0x000000, 0,
      BAGL_GLYPH_ICON_CROSS},
     NULL,
     0,
     0,
     0,
     NULL,
     NULL,
     NULL},
    {{BAGL_ICON, 0x00, 117, 13, 8, 6, 0, 0, 0, 0xFFFFFF, 0x000000, 0,
      BAGL_GLYPH_ICON_CHECK},
     NULL,
     0,
     0,
     0,
     NULL,
     NULL,
     NULL},

    //{{BAGL_ICON                           , 0x01,  31,   9,  14,  14, 0, 0, 0
    //, 0xFFFFFF, 0x000000, 0, BAGL_GLYPH_ICON_EYE_BADGE  }, NULL, 0, 0, 0,
    //NULL, NULL, NULL },
    {{BAGL_LABELINE, 0x01, 0, 12, 128, 12, 0, 0, 0, 0xFFFFFF, 0x000000,
      BAGL_FONT_OPEN_SANS_EXTRABOLD_11px | BAGL_FONT_ALIGNMENT_CENTER, 0},
     "Confirm",
     0,
     0,
     0,
     NULL,
     NULL,
     NULL},
    {{BAGL_LABELINE, 0x01, 0, 26, 128, 12, 0, 0, 0, 0xFFFFFF, 0x000000,
      BAGL_FONT_OPEN_SANS_EXTRABOLD_11px | BAGL_FONT_ALIGNMENT_CENTER, 0},
     "address",
     0,
     0,
     0,
     NULL,
     NULL,
     NULL},

    {{BAGL_LABELINE, 0x02, 0, 12, 128, 12, 0, 0, 0, 0xFFFFFF, 0x000000,
      BAGL_FONT_OPEN_SANS_REGULAR_11px | BAGL_FONT_ALIGNMENT_CENTER, 0},
     "Address",
     0,
     0,
     0,
     NULL,
     NULL,
     NULL},
    {{BAGL_LABELINE, 0x02, 23, 26, 82, 12, 0x80 | 10, 0, 0, 0xFFFFFF, 0x000000,
      BAGL_FONT_OPEN_SANS_EXTRABOLD_11px | BAGL_FONT_ALIGNMENT_CENTER, 26},
     (char *)fullAddress,
     0,
     0,
     0,
     NULL,
     NULL,
     NULL},
};

const bagl_element_t ui_approval_nanos[] = {
    // type                               userid    x    y   w    h  str rad
    // fill      fg        bg      fid iid  txt   touchparams...       ]
    {{BAGL_RECTANGLE, 0x00, 0, 0, 128, 32, 0, 0, BAGL_FILL, 0x000000, 0xFFFFFF,
      0, 0},
     NULL,
     0,
     0,
     0,
     NULL,
     NULL,
     NULL},

    {{BAGL_ICON, 0x00, 3, 12, 7, 7, 0, 0, 0, 0xFFFFFF, 0x000000, 0,
      BAGL_GLYPH_ICON_CROSS},
     NULL,
     0,
     0,
     0,
     NULL,
     NULL,
     NULL},
    {{BAGL_ICON, 0x00, 117, 13, 8, 6, 0, 0, 0, 0xFFFFFF, 0x000000, 0,
      BAGL_GLYPH_ICON_CHECK},
     NULL,
     0,
     0,
     0,
     NULL,
     NULL,
     NULL},

    //{{BAGL_ICON                           , 0x01,  21,   9,  14,  14, 0, 0, 0
    //, 0xFFFFFF, 0x000000, 0, BAGL_GLYPH_ICON_TRANSACTION_BADGE  }, NULL, 0, 0,
    //0, NULL, NULL, NULL },
    {{BAGL_LABELINE, 0x01, 0, 12, 128, 32, 0, 0, 0, 0xFFFFFF, 0x000000,
      BAGL_FONT_OPEN_SANS_EXTRABOLD_11px | BAGL_FONT_ALIGNMENT_CENTER, 0},
     "Confirm",
     0,
     0,
     0,
     NULL,
     NULL,
     NULL},
    {{BAGL_LABELINE, 0x01, 0, 26, 128, 32, 0, 0, 0, 0xFFFFFF, 0x000000,
      BAGL_FONT_OPEN_SANS_EXTRABOLD_11px | BAGL_FONT_ALIGNMENT_CENTER, 0},
     "transaction",
     0,
     0,
     0,
     NULL,
     NULL,
     NULL},

    {{BAGL_LABELINE, 0x03, 0, 12, 128, 32, 0, 0, 0, 0xFFFFFF, 0x000000,
      BAGL_FONT_OPEN_SANS_REGULAR_11px | BAGL_FONT_ALIGNMENT_CENTER, 0},
     "Amount",
     0,
     0,
     0,
     NULL,
     NULL,
     NULL},
    {{BAGL_LABELINE, 0x03, 23, 26, 82, 12, 0x80 | 10, 0, 0, 0xFFFFFF, 0x000000,
      BAGL_FONT_OPEN_SANS_EXTRABOLD_11px | BAGL_FONT_ALIGNMENT_CENTER, 26},
     (char *)fullAmount,
     0,
     0,
     0,
     NULL,
     NULL,
     NULL},

    {{BAGL_LABELINE, 0x04, 0, 12, 128, 32, 0, 0, 0, 0xFFFFFF, 0x000000,
      BAGL_FONT_OPEN_SANS_REGULAR_11px | BAGL_FONT_ALIGNMENT_CENTER, 0},
     "Address",
     0,
     0,
     0,
     NULL,
     NULL,
     NULL},
    {{BAGL_LABELINE, 0x04, 23, 26, 82, 12, 0x80 | 10, 0, 0, 0xFFFFFF, 0x000000,
      BAGL_FONT_OPEN_SANS_EXTRABOLD_11px | BAGL_FONT_ALIGNMENT_CENTER, 50},
     (char *)fullAddress,
     0,
     0,
     0,
     NULL,
     NULL,
     NULL},

    {{BAGL_LABELINE, 0x05, 0, 12, 128, 32, 0, 0, 0, 0xFFFFFF, 0x000000,
      BAGL_FONT_OPEN_SANS_REGULAR_11px | BAGL_FONT_ALIGNMENT_CENTER, 0},
     feeName,
     0,
     0,
     0,
     NULL,
     NULL,
     NULL},
    {{BAGL_LABELINE, 0x05, 23, 26, 82, 12, 0x80 | 10, 0, 0, 0xFFFFFF, 0x000000,
      BAGL_FONT_OPEN_SANS_EXTRABOLD_11px | BAGL_FONT_ALIGNMENT_CENTER, 26},
     (char *)fullFee,
     0,
     0,
     0,
     NULL,
     NULL,
     NULL},

};

static void ui_idle(void);

unsigned int ui_address_prepro(const bagl_element_t *element) {
    if (element->component.userid > 0) {
        unsigned int display = (ux_step == element->component.userid - 1);
        if (display) {
            switch (element->component.userid) {
            case 1:
                UX_CALLBACK_SET_INTERVAL(2000);
                break;
            case 2:
                UX_CALLBACK_SET_INTERVAL(MAX(
                    3000, 1000 + bagl_label_roundtrip_duration_ms(element, 7)));
                break;
            }
        }
        return display;
    }
    return 1;
}

unsigned int io_seproxyhal_touch_address_ok(const bagl_element_t *e) {
    set_result_get_publicKey();
    aio_write16(SW_OK);
    // Send back the response, do not restart the event loop
    io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, g_aio.tx);
    // Display back the original UX
    ui_idle();
    return 0; // do not redraw the widget
}

unsigned int io_seproxyhal_touch_address_cancel(const bagl_element_t *e) {
    aio_write16(SW_USER_DENIAL);
    // Send back the response, do not restart the event loop
    io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, 2);
    // Display back the original UX
    ui_idle();
    return 0; // do not redraw the widget
}

unsigned int ui_address_nanos_button(unsigned int button_mask,
                                     unsigned int button_mask_counter) {
    switch (button_mask) {
    case BUTTON_EVT_RELEASED | BUTTON_LEFT: // CANCEL
        io_seproxyhal_touch_address_cancel(NULL);
        break;

    case BUTTON_EVT_RELEASED | BUTTON_RIGHT: { // OK
        io_seproxyhal_touch_address_ok(NULL);
        break;
    }
    }
    return 0;
}

unsigned int ui_approval_prepro(const bagl_element_t *element) {
    unsigned int display = 1;
    if (element->component.userid > 0) {
        display = (ux_step == element->component.userid - 1);
        if (display) {
            switch (element->component.userid) {
            case 1:
                UX_CALLBACK_SET_INTERVAL(2000);
                break;
            case 2:
                    display = 0;
                    ux_step++; // display the next step
                break;
            case 3:
                UX_CALLBACK_SET_INTERVAL(MAX(
                    3000, 1000 + bagl_label_roundtrip_duration_ms(element, 7)));
                break;
            case 4:
                UX_CALLBACK_SET_INTERVAL(MAX(
                    3000, 1000 + bagl_label_roundtrip_duration_ms(element, 7)));
                break;
            case 5:
                UX_CALLBACK_SET_INTERVAL(MAX(
                    3000, 1000 + bagl_label_roundtrip_duration_ms(element, 7)));
                break;
            }
        }
    }
    return display;
}

unsigned int io_seproxyhal_touch_tx_ok(const bagl_element_t *e) {
    uint8_t privateKeyData[32];
    uint8_t signature[100];
    uint8_t signatureLength;
    cx_ecfp_private_key_t privateKey;
    uint8_t rLength, sLength, rOffset, sOffset;

#if TEST_MODE
    if (tmpCtx.transactionContext.bip32Path[0]==TEST_BIP32) {
        os_memmove(&privateKey, &testPrivateKey, sizeof(privateKey));
    } else 
#endif    
    {
        os_perso_derive_node_bip32(
            CX_CURVE_256K1, tmpCtx.transactionContext.bip32Path,
            tmpCtx.transactionContext.pathLength, privateKeyData, NULL);
        cx_ecfp_init_private_key(CX_CURVE_256K1, privateKeyData, 32, &privateKey);
        os_memset(privateKeyData, 0, sizeof(privateKeyData));
    }

    unsigned int info = 0;
    signatureLength =
        cx_ecdsa_sign(&privateKey, CX_RND_RFC6979 | CX_LAST, CX_SHA256,
                      tmpCtx.transactionContext.hash,
                      sizeof(tmpCtx.transactionContext.hash), signature,
                      sizeof(signature), &info);
    if (info & CX_ECCINFO_PARITY_ODD) {
        signature[0] |= 0x01;
    }

    os_memset(&privateKey, 0, sizeof(privateKey));
    // Parity is present in the sequence tag in the legacy API
    rLength = signature[3];
    sLength = signature[4 + rLength + 1];
    rOffset = (rLength == 33 ? 1 : 0);
    sOffset = (sLength == 33 ? 1 : 0);
    aio_write(signature + 4 + rOffset, 32);
    aio_write(signature + 4 + rLength + 2 + sOffset, 32);
    aio_write8(signature[0] & 0x01);
    aio_write(tmpCtx.transactionContext.hash, 32);
    aio_write16(SW_OK);
    // Send back the response, do not restart the event loop
    io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, g_aio.tx);
    // Display back the original UX
    ui_idle();
    g_isSigning = false;
    return 0; // do not redraw the widget
}

unsigned int io_seproxyhal_touch_tx_cancel(const bagl_element_t *e) {
    aio_write16(SW_USER_DENIAL);
    // Send back the response, do not restart the event loop
    io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, 2);
    // Display back the original UX
    ui_idle();
    g_isSigning = false;
    return 0; // do not redraw the widget
}

unsigned int ui_approval_nanos_button(unsigned int button_mask,
                                      unsigned int button_mask_counter) {
    switch (button_mask) {
    case BUTTON_EVT_RELEASED | BUTTON_LEFT:
        io_seproxyhal_touch_tx_cancel(NULL);
        break;

    case BUTTON_EVT_RELEASED | BUTTON_RIGHT: {
        io_seproxyhal_touch_tx_ok(NULL);
        break;
    }
    }
    return 0;
}

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
            io_seproxyhal_spi_send(g_aio_buf, tx_len);

            if (channel & IO_RESET_AFTER_REPLIED) {
                reset();
            }
            return 0; // nothing received from the master so far (it's a tx
                      // transaction)
        } else {
            return io_seproxyhal_spi_recv(g_aio_buf,
                                          sizeof(g_aio_buf), 0);
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
    case SEPROXYHAL_TAG_TICKER_EVENT:
        UX_TICKER_EVENT(G_io_seproxyhal_spi_buffer, {
            if (UX_ALLOWED) {
                if (skipWarning && (ux_step == 0)) {
                    ux_step++;
                }

                if (ux_step_count) {
                    // prepare next screen
                    ux_step = (ux_step + 1) % ux_step_count;
                    // redisplay screen
                    UX_REDISPLAY();
                }
            }
        });
        break;
    }

    if (!io_seproxyhal_spi_is_status_sent()) {
        io_seproxyhal_general_status();
    }
    return 1;
}

static const uint8_t const HEXDIGITS[] = "0123456789abcdef";
static const uint8_t const MASK[] = {0x80, 0x40, 0x20, 0x10,
                                     0x08, 0x04, 0x02, 0x01};

char convertDigit(uint8_t *address, uint8_t index, uint8_t *hash) {
    unsigned char digit = address[index / 2];
    if ((index % 2) == 0) {
        digit = (digit >> 4) & 0x0f;
    } else {
        digit = digit & 0x0f;
    }
    if (digit < 10) {
        return HEXDIGITS[digit];
    } else {
        unsigned char data = hash[index / 8];
        if (((data & MASK[index % 8]) != 0) && (digit > 9)) {
            return HEXDIGITS[digit] /*- 'a' + 'A'*/;
        } else {
            return HEXDIGITS[digit];
        }
    }
}

void getICONAddressStringFromBinary(uint8_t *address, uint8_t *out,
                                   cx_sha3_t *sha3Context) {
    uint8_t hashChecksum[32];
    uint8_t i;
    cx_sha3_init(sha3Context, 256);
    cx_hash((cx_hash_t *)sha3Context, CX_LAST, address, 20, hashChecksum,
            sizeof(hashChecksum));
    out[0] = 'h';
    out[1] = 'x';
    for (i = 0; i < 40; i++) {
        out[i+2] = convertDigit(address, i, hashChecksum);
    }
    out[42] = '\0';
}

void getICONAddressStringFromKey(cx_ecfp_public_key_t *publicKey, uint8_t *out,
                                cx_sha3_t *sha3Context) {
    uint8_t hashAddress[32];
    cx_sha3_init(sha3Context, 256);
    cx_hash((cx_hash_t *)sha3Context, CX_LAST, publicKey->W + 1, 64,
            hashAddress, sizeof(hashAddress));
    getICONAddressStringFromBinary(hashAddress + 12, out, sha3Context);
}

bool adjustDecimals(char *src, uint32_t srcLength, char *target,
                    uint32_t targetLength, uint8_t decimals) {
    uint32_t startOffset;
    uint32_t lastZeroOffset = 0;
    uint32_t offset = 0;
    if ((srcLength == 1) && (*src == '0')) {
        if (targetLength < 2) {
            return false;
        }
        target[0] = '0';
        target[1] = '\0';
        return true;
    }
    if (srcLength <= decimals) {
        uint32_t delta = decimals - srcLength;
        if (targetLength < srcLength + 1 + 2 + delta) {
            return false;
        }
        target[offset++] = '0';
        target[offset++] = '.';
        for (uint32_t i = 0; i < delta; i++) {
            target[offset++] = '0';
        }
        startOffset = offset;
        for (uint32_t i = 0; i < srcLength; i++) {
            target[offset++] = src[i];
        }
        target[offset] = '\0';
    } else {
        uint32_t sourceOffset = 0;
        uint32_t delta = srcLength - decimals;
        if (targetLength < srcLength + 1 + 1) {
            return false;
        }
        while (offset < delta) {
            target[offset++] = src[sourceOffset++];
        }
        if (decimals != 0) {
            target[offset++] = '.';
        }
        startOffset = offset;
        while (sourceOffset < srcLength) {
            target[offset++] = src[sourceOffset++];
        }
	target[offset] = '\0';
    }
    for (uint32_t i = startOffset; i < offset; i++) {
        if (target[i] == '0') {
            if (lastZeroOffset == 0) {
                lastZeroOffset = i;
            }
        } else {
            lastZeroOffset = 0;
        }
    }
    if (lastZeroOffset != 0) {
        target[lastZeroOffset] = '\0';
        if (target[lastZeroOffset - 1] == '.') {
            target[lastZeroOffset - 1] = '\0';
        }
    }
    return true;
}

void handleGetPublicKey() {
    uint8_t p1 = g_aio_buf[OFFSET_P1];
    uint8_t p2 = g_aio_buf[OFFSET_P2];
    uint8_t* dataBuffer = g_aio_buf + OFFSET_CDATA;

    uint8_t privateKeyData[32];
    uint32_t bip32Path[MAX_BIP32_PATH];
    uint32_t i;
    uint32_t lc = g_aio_buf[OFFSET_LC];
    uint8_t bip32PathLength = *(dataBuffer++);
    cx_ecfp_private_key_t privateKey;

    if (g_isSigning) {
        aio_write16(SW_BAD_STATE);
        return;
    }
    if ((bip32PathLength < 0x01) || (bip32PathLength > MAX_BIP32_PATH)) {
        PRINTF("Invalid path\n");
        aio_write16(SW_BAD_DATA);
        return;
    }
    if (lc < 1+bip32PathLength*4) {
        aio_write16(SW_BAD_LENGTH);
        return;
    }
    if ((p1 != P1_CONFIRM) && (p1 != P1_NON_CONFIRM)) {
        aio_write16(SW_BAD_PARAM);
        return;
    }
    if ((p2 != P2_CHAINCODE) && (p2 != P2_NO_CHAINCODE)) {
        aio_write16(SW_BAD_PARAM);
        return;
    }
    for (i = 0; i < bip32PathLength; i++) {
        bip32Path[i] = (dataBuffer[0] << 24) | (dataBuffer[1] << 16) |
                       (dataBuffer[2] << 8) | (dataBuffer[3]);
        dataBuffer += 4;
    }
    tmpCtx.publicKeyContext.getChaincode = (p2 == P2_CHAINCODE);

#if TEST_MODE
    if (bip32Path[0]==TEST_BIP32) {
        os_memmove(&privateKey, &testPrivateKey, sizeof(privateKey));
        if (tmpCtx.publicKeyContext.getChaincode) {
            os_memset(&tmpCtx.publicKeyContext.chainCode, 0,
                    sizeof(tmpCtx.publicKeyContext.chainCode));
        }
    } else 
#endif    
    {
        os_perso_derive_node_bip32(CX_CURVE_256K1, bip32Path, bip32PathLength,
                                   privateKeyData,
                                   (tmpCtx.publicKeyContext.getChaincode
                                        ? tmpCtx.publicKeyContext.chainCode
                                        : NULL));
        cx_ecfp_init_private_key(CX_CURVE_256K1, privateKeyData, 32, &privateKey);
    }
    cx_ecfp_generate_pair(CX_CURVE_256K1, &tmpCtx.publicKeyContext.publicKey,
                          &privateKey, 1);
    os_memset(&privateKey, 0, sizeof(privateKey));
    os_memset(privateKeyData, 0, sizeof(privateKeyData));
    getICONAddressStringFromKey(&tmpCtx.publicKeyContext.publicKey,
                               tmpCtx.publicKeyContext.address, &sha3);
    if (p1 == P1_NON_CONFIRM) {
        set_result_get_publicKey();
        aio_write16(SW_OK);
    } else {
        // prepare for a UI based reply
        skipWarning = false;
        os_memmove(fullAddress, tmpCtx.publicKeyContext.address, 43);
        fullAddress[42] = ' ';
        fullAddress[43] = '\0';
        ux_step = 0;
        ux_step_count = 2;
        UX_DISPLAY(ui_address_nanos, ui_address_prepro);

        g_aio.flags |= IO_ASYNCH_REPLY;
    }
}

static int hexValue(char digit) {
    if (digit < '0')
        return -1;
    if (digit <= '9')
        return digit-'0';
    if (digit < 'a')
        return -1;
    if (digit <= 'f')
        return digit-'a'+10;
    return -1;
}

bool parseHex32(const char* hex, int len, uint32_t* target) {
    *target = 0;

    for (int i=0; i<len; ++i) {
        int d = hexValue(hex[i]);
        if (d<0)
            return false;
        *target = (*target<<4) | d;
    }
    return true;
}

bool parseHex256(const char* hex, int len, uint256_t* target) {
    uint256_t v;

    if (len>64)
        return false;

    clear256(&v);
    clear256(target);

    for (int i=0; i<len; ++i) {
        int d = hexValue(hex[i]);
        if (d<0)
            return false;
        shiftl256(target, 4, target);
        LOWER(LOWER(v)) = d;
        or256(target, &v, target);
    }
    return true;
}

static int onPartialToken(Parser* parser, const char* partial, int len) {
    if (parser->state==PARSER_STATE_PREFIX) {
        return -1;
    } else if (parser->state&PARSER_STATE_KEY_FLAG) {
        parser->state = PARSER_STATE_KEY_IGNORE;
    } else if (parser->state&PARSER_STATE_VALUE_FLAG) {
        parser->state = PARSER_STATE_VALUE_IGNORE;
    }
    return 0;
}

bool isValidHex(const char* p, int len) {
    if (p[0]!='0' || p[1]!='x')
        return false;
    for (int i=2; i<len; ++i)
        if ( hexValue(p[i])<0 )
            return false;
    return true;
}

static int onTokenEnd(Parser* parser, const char* token, int len) {
    if (parser->state==PARSER_STATE_PREFIX) {
        if (len!=19 || os_memcmp(token, "icx_sendTransaction", 19)!=0)
            return -1;
    } else if (parser->state==PARSER_STATE_KEY) {
        if (len==3 && os_memcmp(token, "fee", 3)==0) {
            parser->state = PARSER_STATE_VALUE_FEE;
        } else if (len==9 && os_memcmp(token, "stepLimit", 9)==0) {
            parser->state = PARSER_STATE_VALUE_STEP_LIMIT;
        } else if (len==5 && os_memcmp(token, "value", 5)==0) {
            parser->state = PARSER_STATE_VALUE_VALUE;
        } else if (len==7 && os_memcmp(token, "version", 7)==0) {
            parser->state = PARSER_STATE_VALUE_VERSION;
        } else if (len==2 && os_memcmp(token, "to", 2)==0) {
            parser->state = PARSER_STATE_VALUE_TO;
        } else {
            parser->state = PARSER_STATE_VALUE_IGNORE;
        }
        return 0;
    } else if (parser->state==PARSER_STATE_VALUE_FEE) {
        if (len > MAX_VALUE_LEN)
            return -1;
        if (!isValidHex(token, len))
            return -1;
        parser->hasFee = true;
        parseHex256(token+2, len-2, &parser->fee);
    } else if (parser->state==PARSER_STATE_VALUE_STEP_LIMIT) {
        if (len > MAX_VALUE_LEN)
            return -1;
        if (!isValidHex(token, len))
            return -1;
        parser->hasStepLimit = true;
        parseHex256(token+2, len-2, &parser->stepLimit);
    } else if (parser->state==PARSER_STATE_VALUE_VALUE) {
        if (len > MAX_VALUE_LEN)
            return -1;
        if (!isValidHex(token, len))
            return -1;
        parser->hasValue = true;
        parseHex256(token+2, len-2, &parser->value);
    } else if (parser->state==PARSER_STATE_VALUE_VERSION) {
        if (len > 10)
            return -1;
        if (!isValidHex(token, len))
            return -1;
        parser->hasVersion = true;
        parseHex32(token+2, len-2, &parser->version);
    } else if (parser->state==PARSER_STATE_VALUE_TO) {
        if (len > 42)
            return -1;
        parser->hasTo = true;
        os_memmove(parser->to, token, 43);
    }
    parser->state = PARSER_STATE_KEY;
    return 0;
}

void parser_init(Parser* parser) {
    parser->state = PARSER_STATE_PREFIX;
    parser->nestingLevel = 0;
    parser->isEscaping = true;
    parser->wp = parser->buf;
    parser->hasFee = false;
    clear256(&parser->fee);
    parser->hasStepLimit = false;
    clear256(&parser->stepLimit);
    parser->hasTo = false;
    parser->to[0] = 0;
    parser->hasValue = false;
    clear256(&parser->value);
    parser->hasVersion = false;
    parser->version = 0;
}

inline const char* parser_endOfBuf(Parser* parser) {
    return parser->buf + PARSER_BUF_SIZE;
}

int parser_feed(Parser* parser, const uint8_t *p, int len) {
    const uint8_t* end = p+len;

    while (p<end) {
        if (parser->isEscaping) {
            parser->isEscaping = false;
        } else {
            if (*p=='\\') {
                parser->isEscaping = true;
                ++p;
                continue;
            } else if (*p=='.' && parser->nestingLevel==0) {
                *parser->wp = 0;
                ++p;
                if (onTokenEnd(parser, parser->buf, parser->wp - parser->buf)<0)
                    return -1;
                parser->wp = parser->buf;
                continue;
            } else if (*p=='{' || *p=='[') {
                ++(parser->nestingLevel);
                ++p;
                continue;
            } else if (*p=='}' || *p==']') {
                --(parser->nestingLevel);
                ++p;
                continue;
            }
        }
        *(parser->wp)++ = *p++;
        if (parser->wp == parser->buf + PARSER_BUF_SIZE) {
            if (onPartialToken(parser, parser->buf, parser->wp - parser->buf)<0)
                return -1;
            parser->wp = parser->buf;
        }
    }
    return 0;
}

int parser_endFeed(Parser* parser) {
    if (parser->wp > parser->buf) {
        *parser->wp = 0;
        if (onTokenEnd(parser, parser->buf, parser->wp - parser->buf)<0)
            return -1;
    }
    return 0;
}

void handleSign() {
    uint8_t p1 = g_aio_buf[OFFSET_P1];
    uint8_t p2 = g_aio_buf[OFFSET_P2];
    uint8_t* workBuffer = g_aio_buf + OFFSET_CDATA;
    uint16_t dataLength = g_aio_buf[OFFSET_LC];

    uint32_t i;
    if (p1 == P1_FIRST) {
        tmpCtx.transactionContext.pathLength = workBuffer[0];
        if ((tmpCtx.transactionContext.pathLength < 0x01) ||
            (tmpCtx.transactionContext.pathLength > MAX_BIP32_PATH)) {
            PRINTF("Invalid path\n");
            aio_write16(SW_BAD_DATA);
            g_isSigning = false;
            return;
        }
        if (dataLength < 1+tmpCtx.transactionContext.pathLength*4+4) {
            aio_write16(SW_BAD_LENGTH);
            g_isSigning = false;
            return;
        }
        workBuffer++;
        dataLength--;
        for (i = 0; i < tmpCtx.transactionContext.pathLength; i++) {
            tmpCtx.transactionContext.bip32Path[i] =
                (workBuffer[0] << 24) | (workBuffer[1] << 16) |
                (workBuffer[2] << 8) | (workBuffer[3]);
            workBuffer += 4;
            dataLength -= 4;
        }
        tmpCtx.transactionContext.txLeft =
                (workBuffer[0] << 24) | (workBuffer[1] << 16) |
                (workBuffer[2] << 8) | (workBuffer[3]);
        if (tmpCtx.transactionContext.txLeft==0) {
            aio_write16(SW_BAD_LENGTH);
            g_isSigning = false;
            return;
        }
        workBuffer += 4;
        dataLength -= 4;
        cx_sha3_init(&sha3, 256);
        parser_init(&tmpCtx.transactionContext.parser);
    } else if (p1 != P1_MORE) {
        aio_write16(SW_BAD_PARAM);
        g_isSigning = false;
        return;
    }
    if (p1 == P1_MORE && !g_isSigning) {
        aio_write16(SW_BAD_STATE);
        return;
    }
    if (p2 != 0) {
        aio_write16(SW_BAD_PARAM);
        g_isSigning = false;
        return;
    }

    if (dataLength > tmpCtx.transactionContext.txLeft)
        dataLength = tmpCtx.transactionContext.txLeft;
    cx_hash((cx_hash_t *)&sha3, 0, workBuffer, dataLength, NULL, 0);
    int feedRes = parser_feed(&tmpCtx.transactionContext.parser,
            workBuffer, dataLength);
    if (feedRes<0) {
        aio_write16(SW_BAD_PARAM);
        g_isSigning = false;
        return;
    }
    tmpCtx.transactionContext.txLeft -= dataLength;

    if (tmpCtx.transactionContext.txLeft>0) {
        aio_write16(SW_OK);
        g_isSigning = true;
        return;
    }
    feedRes = parser_endFeed(&tmpCtx.transactionContext.parser);
    if (feedRes<0) {
        aio_write16(SW_BAD_PARAM);
        g_isSigning = false;
        return;
    }

    Parser* parser = &tmpCtx.transactionContext.parser;
    if (!parser->hasTo) {
        aio_write16(SW_BAD_DATA);
        g_isSigning = false;
        return;
    }
    uint32_t version = 0x02;
    if (parser->hasVersion)
        version = parser->version;
    if (version<0x02) {
        aio_write16(SW_BAD_DATA);
        g_isSigning = false;
        return;
    }
    if ((version>=0x03 && !parser->hasStepLimit) ||
            (version==0x02 && !parser->hasFee)) {
        aio_write16(SW_BAD_DATA);
        g_isSigning = false;
        return;
    }

    cx_hash((cx_hash_t *)&sha3, CX_LAST, workBuffer, 0,
            tmpCtx.transactionContext.hash,
            sizeof(tmpCtx.transactionContext.hash));

    os_memmove(fullAddress, (unsigned char *)parser->to, 43);
    fullAddress[42] = ' ';
    fullAddress[43] = '\0';

    if(!tostring256(&parser->value, 10, (char *)(g_aio_buf + 100), 100)){
        aio_write16(SW_BAD_DATA);
        g_isSigning = false;
        return;
    }

    i = 0;
    while (g_aio_buf[100 + i] && i <= 100) {
        i++;
    }
    fullAmount[0] = 'I';
    fullAmount[1] = 'C';
    fullAmount[2] = 'X';
    fullAmount[3] = ' ';
    bool res;
    res = adjustDecimals((char *)(g_aio_buf + 100), i,
                            fullAmount+4, sizeof(fullAmount)-4, ICX_EXP);
    if (!res) {
        aio_write16(SW_BAD_DATA);
        g_isSigning = false;
        return;
    }


    if (version>=0x03) {
        os_memmove(feeName, "StepLimit", 10);
        if(!tostring256(&parser->stepLimit, 10, fullFee, sizeof(fullFee))){
            aio_write16(SW_BAD_DATA);
            g_isSigning = false;
            return;
        }
    } else {
        os_memmove(feeName, "Fee", 4);
        if(!tostring256(&parser->fee, 10, (char *)(g_aio_buf + 100), 100)){
            aio_write16(SW_BAD_DATA);
            g_isSigning = false;
            return;
        }
        i = 0;
        while (g_aio_buf[100 + i] && i <= 100) {
            i++;
        }
        if (version==0x02) {
            fullFee[0] = 'I';
            fullFee[1] = 'C';
            fullFee[2] = 'X';
            fullFee[3] = ' ';
        }
        res = adjustDecimals((char *)(g_aio_buf + 100), i,
                                fullFee+4, sizeof(fullFee)-4, ICX_EXP);
        if (!res) {
            aio_write16(SW_BAD_DATA);
            g_isSigning = false;
            return;
        }
    }

    skipWarning = true;
    ux_step = 0;
    ux_step_count = 5;
    UX_DISPLAY(ui_approval_nanos, ui_approval_prepro);

    g_aio.flags |= IO_ASYNCH_REPLY;
}

void handleGetAppConfiguration() {
    aio_write8(LEDGER_MAJOR_VERSION);
    aio_write8(LEDGER_MINOR_VERSION);
    aio_write8(LEDGER_PATCH_VERSION);
    aio_write16(SW_OK);
}

#if TEST_MODE
void handleSetTestPrivateKey() {
    if (g_aio_buf[OFFSET_LC]!=32) {
        aio_write16(SW_BAD_LENGTH);
        return;
    }
    cx_ecfp_init_private_key(CX_CURVE_256K1, g_aio_buf+OFFSET_CDATA, 32,
            &testPrivateKey);
    aio_write16(SW_OK);
}
#endif

void app_main(void)
{
    g_isSigning = false;
    g_aio.flags = 0;
    g_aio.rx = 0;
    g_aio.tx = 0;
    while (true) {
        BEGIN_TRY {
            TRY {
                g_aio.rx = io_exchange(CHANNEL_APDU | g_aio.flags, g_aio.tx);
                g_aio.flags = 0;
                g_aio.tx = 0;

                // no apdu received
                if (g_aio.rx == 0) {
                    aio_write16(SW_NO_APDU);
                    continue;
                }

                if (g_aio_buf[OFFSET_CLA] != CLA) {
                    aio_write16(SW_BAD_CLA);
                    continue;
                }

                const uint8_t ins = g_aio_buf[OFFSET_INS];
                if (ins==INS_GET_PUBLIC_KEY) {
                    handleGetPublicKey();
                } else if (ins==INS_SIGN) {
                    handleSign();
                } else if (ins==INS_GET_APP_CONFIGURATION) {
                    handleGetAppConfiguration();
#if TEST_MODE
                } else if (ins==INS_SET_TEST_PRIVATE_KEY) {
                    handleSetTestPrivateKey();
#endif
                } else {
                    aio_write16(SW_BAD_INST);
                }
            }
            CATCH(EXCEPTION_IO_RESET) {
                THROW(EXCEPTION_IO_RESET);
            }
            CATCH_OTHER(e) {
                g_aio.tx = 0;
                aio_write16(SW_INTERNAL_ERROR_MASK | (e&0x0FF));
            }
            FINALLY {
            }
        }
        END_TRY;
    }
}

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
