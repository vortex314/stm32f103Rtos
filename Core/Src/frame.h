
#ifndef _FRAME_H_
#define _FRAME_H_
#include <stdint.h>
#include <vector>
#include "log.h"

typedef std::vector<uint8_t> Bytes;

#define PPP_FCS_SIZE 2
// PPP special characters
#define PPP_MASK_CHAR 0x20
#define PPP_ESC_CHAR 0x7D
#define PPP_FLAG_CHAR 0x7E

Bytes frame(const Bytes &in);
bool deframe(Bytes &out, const Bytes &in);
#endif
