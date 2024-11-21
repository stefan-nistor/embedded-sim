#ifndef PARSER_H
#define PARSER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <model/Instruction.h>

typedef enum {
  PARSER_ERROR_NONE,
  PARSER_ERROR_ILLEGAL_PARAMETER,
  PARSER_ERROR_INVALID_PATH,
  PARSER_ERROR_ARRAY_TOO_SMALL,
  PARSER_ERROR_INVALID_TOKEN,
  PARSER_ERROR_UNDEFINED_REFERENCE,
  PARSER_ERROR_UNKNOWN,
} ParserError;

typedef enum {
  PARSER_INPUT_TYPE_CODE,
  PARSER_INPUT_TYPE_FILE_PATH,
} ParserInputType;

typedef struct {
  StructureType structureType;
  void* pNext;
  ParserInputType inputType;
  U32 dataLength;
  char const* pData;
} ParserCreateInfo;

typedef struct {
  U32 registerNameLength;
  char const* pRegisterName;
  Register2 * pRegister;
} ParserMappedRegister;

typedef struct {
  StructureType structureType;
  void* pNext;
  U16 mappedRegisterCount;
  ParserMappedRegister const* pMappedRegisters;
} ParserGetInstructionSetInfo;

typedef struct {
  StructureType structureType;
  void* pNext;
  U32 line;
  U32 column;
  U32 tokenLength;
  char* pToken;
} ParserInvalidTokenOutputInfo;

typedef struct {
  StructureType structureType;
  void* pNext;
  U32 referencingInstructionIndex;
  U32 tokenLength;
  char* pToken;
} ParserUndefinedReferenceOutputInfo;

DEFINE_HANDLE(Parser);

extern ParserError createParser(ParserCreateInfo const* pCreateInfo, Parser* pParser);
extern void destroyParser(Parser parser);

extern ParserError getParserInstructionSet(
    Parser parser,
    ParserGetInstructionSetInfo const* pGetInfo,
    U16* pInstructionCount,
    Instruction* pInstructions
);

#ifdef __cplusplus
}
#endif

#endif
