//
// Created by rosa on 11/5/24.
//

#ifndef INSTRUCTION_H
#define INSTRUCTION_H

#include <model/InstructionType.h>
#include <model/Register.h>
#include <model/parameter/Parameter.h>

typedef struct Private_Instruction * Instruction;

extern Instruction Instruction_ctor3(InstructionType type, Parameter p1, Parameter p2);

static inline Instruction Instruction_ctor() {
  return Instruction_ctor3(DEFAULT, NULL, NULL);
}

static inline Instruction Instruction_ctor1(InstructionType type) {
  return Instruction_ctor3(type, NULL, NULL);
}

static inline Instruction Instruction_ctor2(InstructionType type, Parameter p1) {
  return Instruction_ctor3(type, p1, NULL);
}

extern void Instruction_dtor(Instruction obj);

extern InstructionType Instruction_getType(Instruction self);
extern void Instruction_setType(Instruction self, InstructionType type);

extern Parameter  Instruction_getParam1(Instruction self);
extern Parameter Instruction_getParam2(Instruction self);

extern void Instruction_setParam1(Instruction self, Parameter param1);
extern void Instruction_setParam2(Instruction self, Parameter param2);

extern void Instruction_setParameters(Instruction self, Parameter p1, Parameter p2);

extern bool Instruction_isALU(Instruction self);
extern bool Instruction_isIPU(Instruction self);
extern bool Instruction_isMMU(Instruction self);

#endif //INSTRUCTION_H
