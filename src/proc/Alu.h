//
// Created by rosa on 11/5/24.
//

#ifndef ALU_H
#define ALU_H
#include <model/Instruction.h>

typedef struct Private_ALU * ALU;

extern ALU ALU_ctor(Register*, Register*);
extern void ALU_dtor(ALU obj);
extern void ALU_execute(ALU self, Instruction instruction);
#endif //ALU_H
