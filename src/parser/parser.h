#ifndef PARSER_H
#define PARSER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <model/Instruction.h>

typedef struct Parser_Handle* Parser;

extern Parser Parser_fromPath(char const* path);
extern Parser Parser_fromCode(char const* code);
extern Parser Parser_fromSizedCode(size_t length, char const* code);
extern void Parser_destruct(Parser parser);

extern U16 Parser_getInstructionSet(Parser parser, Instruction const** ppInstructionList /*, CPUMap? */);

#ifdef __cplusplus
}
#endif

#endif
