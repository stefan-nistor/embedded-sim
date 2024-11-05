//
// Created by rosa on 11/5/24.
//

#ifndef INSTRUCTIONTYPE_H
#define INSTRUCTIONTYPE_H

typedef enum {
  DEFAULT,
  ALU_ADD, ALU_SUB, ALU_MUL, ALU_DIV,
  ALU_AND, ALU_OR, ALU_XOR, ALU_NOT,
  ALU_SHL, ALU_SHR,
  ALU_CMP,

  IPU_JMP, IPU_JEQ, IPU_JNE, IPU_JLT, IPU_JLE, IPU_JGT, IPU_JGE, IPU_CALL, IPU_RET,
  MMU_MOV, MMU_PUSH, MMU_POP
} InstructionType;

#endif //INSTRUCTIONTYPE_H
