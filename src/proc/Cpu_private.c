#include <assert.h>
#include <stdlib.h>
#include <proc/Cpu.h>

typedef struct Private_CPU {
  Register2 dataRegisters[CPU_DATA_REGISTRY_LIST_SIZE];
  Register2 flagRegister;
  ALU alu;
} Private_CPU;

Private_CPU * CPU_ctor() {
  Private_CPU * cpu = (Private_CPU *) malloc(sizeof(Private_CPU));

  for(int i = 0; i < CPU_DATA_REGISTRY_LIST_SIZE; i++) {
    cpu->dataRegisters[i] = 0;
  }
  cpu->flagRegister = 0;
  return cpu;
}

void CPU_setDataRegister(Private_CPU * self, U8 index, Register2 value) {
  assert(index >= 0 && index <= CPU_DATA_REGISTRY_LIST_SIZE && "Index out of bounds.\n");
  self->dataRegisters[index] = value;
}

U16* CPU_getDataRegister(Private_CPU * self, U8 index) {
  assert(index >= 0 && index <= CPU_DATA_REGISTRY_LIST_SIZE && "Index out of bounds.\n");
  return &self->dataRegisters[index];
}

void CPU_dtor(Private_CPU * self) {
  free(self);
}

void CPU_setALU(CPU self, ALU alu) {
  assert(self != NULL && alu != NULL);
  self->alu = alu;
}

static void CPU_prepareStateBefore(Private_CPU * self, Instruction instr) {
  assert(instr != NULL);
  if(!Instruction_isIPU(instr)){
   self->flagRegister = 0;
  }
}

void CPU_raiseFlag(Private_CPU * self, U16 flag) {
  self->flagRegister |= flag;
}

Register2 CPU_getFlagRegister(Private_CPU * self) {
  return self->flagRegister;
}

Register2 * CPU_getDataRegisters(Private_CPU * self) {
  return self->dataRegisters;
}

void CPU_execute(Private_CPU * self, Instruction instr) {
  assert(instr != NULL);
  CPU_prepareStateBefore(self, instr);

  if(Instruction_isALU(instr)) {
    ALU_execute(self->alu, instr);
  }
}