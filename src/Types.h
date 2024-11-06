//
// Created by rosa on 11/5/24.
//

#ifndef TYPES_H
#define TYPES_H

#include <stdbool.h>

#ifndef NULL
#define NULL ((void*)(0))
#endif
#define CPU_DATA_REGISTRY_LIST_SIZE (8)
#define MAX_INSTR_LIST_SIZE (1024)

////////////////////////////
///  FR = Flag Register  ///
////////////////////////////

#define FR_OVERFLOW_FLAG   (0x0001)
#define FR_ZERO_FLAG       (0x0002)
#define FR_DIV_ZERO_FLAG   (0x0004)
#define FR_EQUAL_FLAG      (0x0008)
#define FR_LESS_FLAG       (0x0010)
#define FR_ILLEGAL_FLAG    (0x0020)
#define FR_SEG_FLAG        (0x0040)
#define FR_MULTISTATE_FLAG (0x0080)

#define FR_BITMASK (FR_OVERFLOW_FLAG | FR_ZERO_FLAG | FR_DIV_ZERO_FLAG | FR_EQUAL_FLAG \
                    | FR_LESS_FLAG | FR_ILLEGAL_FLAG | FR_SEG_FLAG | FR_MULTISTATE_FLAG)


typedef unsigned char U8;
typedef unsigned short int U16;
typedef unsigned int U32;

#endif //TYPES_H
