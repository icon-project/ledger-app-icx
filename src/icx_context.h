#ifndef __ICX_CONTEXT_H__
#define __ICX_CONTEXT_H__

#include <stdint.h>

typedef struct icx_context_s {
    unsigned char io_flags;
    unsigned short in_length;
    unsigned short out_length;
} icx_context_t;

extern icx_context_t G_icx_context;

#endif