//
// Created by rosa on 11/6/24.
//

#include <gtest/gtest.h>

extern "C" {
#include <model/Register.h>
#include <proc/Alu.h>
#include <proc/Cpu.h>
}

TEST(CpuTest, Init) {
  auto cpu = CPU_ctor();
  auto dataRegs = CPU_getDataRegisters(cpu);
  for(int i = 0; i < CPU_DATA_REGISTRY_LIST_SIZE; i++){
    ASSERT_EQ(0, dataRegs[i]);
  }
  ASSERT_EQ(0, CPU_getFlagRegister(cpu));
  CPU_dtor(cpu);
}

TEST(CpuTest, raiseFLag) {
  auto cpu = CPU_ctor();
  CPU_raiseFlag(cpu, FR_SEG_FLAG);
  ASSERT_EQ(FR_SEG_FLAG, CPU_getFlagRegister(cpu));
  CPU_dtor(cpu);
}

TEST(CpuTest, setALU) {
  auto cpu = CPU_ctor();
  Register2 flg = 0;
  Register2 ovf = 0;
  ALU alu = ALU_ctor(&flg, &ovf);

  CPU_setALU(cpu, alu);

  ALU_dtor(alu);
  CPU_dtor(cpu);
}

TEST(CpuTest, executeALU) {
  Register2 flg = 0;
  Register2 ovf = 0;
  auto alu = ALU_ctor(&flg, &ovf);
  auto cpu = CPU_ctor();
  CPU_setDataRegister(cpu, 0, 23);
  CPU_setDataRegister(cpu, 1, 6);
  auto p0 = CPU_getDataRegister(cpu, 0);
  auto p1 = CPU_getDataRegister(cpu, 1);
  auto instr = Instruction_ctor3(ALU_ADD, createRegister(p0), createRegister(p1));

  CPU_setALU(cpu, alu);
  CPU_execute(cpu, instr);

  ASSERT_EQ(29, *CPU_getDataRegister(cpu, 0));

  Instruction_dtor(instr);
  CPU_dtor(cpu);
  ALU_dtor(alu);
}
