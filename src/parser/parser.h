#ifndef PARSER_H
#define PARSER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>

typedef struct Parser_Handle* Parser;

extern Parser Parser_fromPath(char const* path);
extern Parser Parser_fromCode(char const* code);
extern Parser Parser_fromSizedCode(size_t length, char const* code);
extern void Parser_destruct(Parser parser);


#ifdef __cplusplus
}
#endif

#endif
