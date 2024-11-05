//
// Created by rosa on 11/5/24.
//
#include <model/Instruction.h>
#include <stdlib.h>

typedef struct Private_Instruction {
  InstructionType type;
  Register *param1;
  Register *param2;

} Private_Instruction;

Private_Instruction *Instruction_ctor3(InstructionType type, Register *p1, Register *p2) {
  Private_Instruction *instr = (Private_Instruction *) malloc(sizeof(Private_Instruction));
  instr->type = type;
  instr->param1 = p1;
  instr->param2 = p2;
  return instr;
}

void Instruction_dtor(Private_Instruction *self) { free(self); }

InstructionType Instruction_getType(Private_Instruction *self) { return self->type; }

void Instruction_setType(Private_Instruction *self, InstructionType type) { self->type = type; }

Register *Instruction_getParam1(Private_Instruction *self) { return self->param1; }

Register *Instruction_getParam2(Private_Instruction *self) { return self->param2; }

void Instruction_setParam1(Instruction self, Register *param1) { self->param1 = param1; }

void Instruction_setParam2(Instruction self, Register *param2) { self->param2 = param2; }

bool Instruction_isALU(Private_Instruction *self) {
  if (self->type >= ALU_ADD && self->type <= ALU_CMP) {
    return true;
  }
  return false;
}

bool Instruction_isIPU(Private_Instruction *self) {
  if (self->type >= IPU_JMP && self->type <= IPU_RET) {
    return true;
  }
  return false;
}

bool Instruction_isMMU(Private_Instruction *self) {
  if (self->type >= MMU_MOV && self->type <= MMU_POP) {
    return true;
  }
  return false;
}
