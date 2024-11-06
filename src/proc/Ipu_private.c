//
// Created by rosa on 11/5/24.
//

#include <Types.h>
#include <assert.h>
#include <stdlib.h>
#include "model/Instruction.h"
#include "model/Register.h"

typedef struct Private_IPU {

  Register *flagRegister;
  Register *programCounter;

} Private_IPU;

Private_IPU * IPU_ctor(Instruction *instructions, U16 instrCount, Register *flagRegister, Register * programCounter) {
  Private_IPU * ipu = (Private_IPU*) malloc(sizeof(Private_IPU));
  ipu->flagRegister = flagRegister;
  ipu->programCounter = programCounter;
  return ipu;
}

void IPU_dtor(Private_IPU * self) {
  free(self);
}

static void IPU_jump(Private_IPU * self, Register * at) {
  assert(at != NULL);
  self->programCounter = at - 1;
}

static void IPU_conditionedJump(Private_IPU* self, bool condition, Register * at) {
  if(condition) {
    IPU_jump(self, at);
  }
}

void IPU_execute(Private_IPU * self, Instruction instruction) {
  InstructionType type = Instruction_getType(instruction);
  Register *at = Instruction_getParam1(instruction);

  switch (type) {
    case IPU_JMP:
      IPU_jump(self, at);
      break;
    case IPU_JNE:
      IPU_conditionedJump(self, !Register_isSet(*self->flagRegister, FR_EQUAL_FLAG), at);
      break;
    case IPU_JEQ:
      IPU_conditionedJump(self, Register_isSet(*self->flagRegister, FR_EQUAL_FLAG), at);
      break;
    case IPU_JLT:
      IPU_conditionedJump(self, Register_isSet(*self->flagRegister, FR_LESS_FLAG)
                          && !Register_isSet(*self->flagRegister, FR_EQUAL_FLAG),
                          at);
      break;
    case IPU_JGT:
      IPU_conditionedJump(self, !Register_isSet(*self->flagRegister, FR_LESS_FLAG)
                          && !Register_isSet(*self->flagRegister, FR_EQUAL_FLAG),
                          at);
      break;
    case IPU_JLE:
      IPU_conditionedJump(self, Register_isSet(*self->flagRegister, FR_LESS_FLAG)
                          || Register_isSet(*self->flagRegister, FR_EQUAL_FLAG),
                          at);
      break;
    case IPU_JGE:
      IPU_conditionedJump(self, !Register_isSet(*self->flagRegister, FR_LESS_FLAG)
                          || Register_isSet(*self->flagRegister, FR_EQUAL_FLAG),
                          at);
      break;
    case IPU_RET:
    case IPU_CALL:
      assert(false && "Not implemented yet.");
    default:
      assert(false && "Invalid instruction type,");
  }



}