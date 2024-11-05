//
// Created by rosa on 11/5/24.
//

#include <gtest/gtest.h>

extern "C" {
#include <model/Register.h>
#include <proc/Alu.h>
}

namespace {
template <typename Fn, typename... Args> concept Callable = std::is_invocable_v<Fn, Args...>;

template <Callable<ALU&, Register&, Register&> C> auto aluTest(C&& callable) {
  Register flg = 0;
  Register ovf = 0;
  auto alu = ALU_ctor(&flg, &ovf);
  std::invoke(std::forward<C>(callable), alu, flg, ovf);
}
}

TEST(AluTest, Init) {
  Register flg = 0;
  Register ovf = 0;
  ALU alu = ALU_ctor(&flg, &ovf);

  ALU_dtor(alu);
}

TEST(AluTest, ALU_add) {
  aluTest([](ALU& alu, Register& flg, Register& ovf) {
    Register p0 = 1;
    Register p1 = 2;
    auto instr = Instruction_ctor3(ALU_ADD, &p0, &p1);
    ALU_execute(alu, instr);
    Instruction_dtor(instr);
    ASSERT_EQ(3, p0);

  });
}

TEST(AluTest, ALU_sub) {
  aluTest([](ALU& alu, Register& flg, Register& ovf) {
    Register p0 = 2;
    Register p1 = 1;
    auto instr = Instruction_ctor3(ALU_SUB, &p0, &p1);
    ALU_execute(alu, instr);
    Instruction_dtor(instr);
    ASSERT_EQ(1, p0);

  });
}

TEST(AluTest, ALU_mul) {
  aluTest([](ALU& alu, Register& flg, Register& ovf) {
    Register p0 = 2;
    Register p1 = 2;
    auto instr = Instruction_ctor3(ALU_MUL, &p0, &p1);
    ALU_execute(alu, instr);
    Instruction_dtor(instr);
    ASSERT_EQ(4, p0);

  });
}

TEST(AluTest, ALU_div) {
  aluTest([](ALU& alu, Register& flg, Register& ovf) {
    Register p0 = 8;
    Register p1 = 2;
    auto instr = Instruction_ctor3(ALU_DIV, &p0, &p1);
    ALU_execute(alu, instr);
    Instruction_dtor(instr);
    ASSERT_EQ(4, p0);

  });
}

TEST(AluTest, ALU_div_with_remainder) {
  aluTest([](ALU& alu, Register& flg, Register& ovf) {
    Register p0 = 9;
    Register p1 = 2;
    auto instr = Instruction_ctor3(ALU_DIV, &p0, &p1);
    ALU_execute(alu, instr);
    Instruction_dtor(instr);
    ASSERT_EQ(4, p0);
    ASSERT_EQ(1, ovf);
  });
}

TEST(AluTest, ALU_zero_div_raise_flag) {
  aluTest([](ALU& alu, Register& flg, Register& ovf) {
    Register p0 = 9;
    Register p1 = 0;
    auto instr = Instruction_ctor3(ALU_DIV, &p0, &p1);
    ALU_execute(alu, instr);
    Instruction_dtor(instr);
    ASSERT_EQ(9, p0);
    ASSERT_EQ(FR_DIV_ZERO_FLAG, flg);
  });
}
