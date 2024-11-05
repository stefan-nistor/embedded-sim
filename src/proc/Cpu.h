//
// Created by rosa on 11/5/24.
//

#ifndef EMBEDDED_SIM_CPU_H
#define EMBEDDED_SIM_CPU_H

#include <model/Register.h>
#include <model/Instruction.h>
#include <proc/Alu.h>

typedef struct Private_CPU * CPU;

extern CPU CPU_ctor();
extern void CPU_dtor(CPU self);

extern void CPU_setALU(CPU self, ALU alu);
extern void CPU_execute(CPU self, Instruction);
extern void CPU_setDataRegister(CPU self, U8 index, Register value);
extern Register CPU_getDataRegister(CPU self, U8 index);
extern void CPU_raiseFlag(CPU self, U16 flag);
extern Register CPU_getFlagRegister(CPU self);
extern Register * CPU_getDataRegisters(CPU self);

#endif // EMBEDDED_SIM_CPU_H
