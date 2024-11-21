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

////////////////////////////
///  FR = Flag Register2  ///
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

#define ILLEGAL_OPERATION "Illegal operation."
#define INVALID_INSTR "Invalid instruction type."

typedef unsigned char U8;
typedef unsigned short int U16;
typedef unsigned int U32;

typedef enum {
  STRUCTURE_TYPE_PARSER_CREATE_INFO,
  STRUCTURE_TYPE_PARSER_GET_INSTRUCTION_SET_INFO,
  STRUCTURE_TYPE_PARSER_INVALID_TOKEN_OUTPUT_INFO,
  STRUCTURE_TYPE_PARSER_UNDEFINED_REFERENCE_OUTPUT_INFO,
} StructureType;

typedef struct {
  StructureType structureType;
  void const* pNext;
} InStructure;

typedef struct {
  StructureType structureType;
  void* pNext;
} OutStructure;

typedef enum {
  PT_MEMORY_LOCATION,
  PT_REGISTER,
  PT_CONSTANT,
} ParameterType;

#define DEFINE_HANDLE(_handle) typedef struct _handle##_T* _handle

#endif //TYPES_H
