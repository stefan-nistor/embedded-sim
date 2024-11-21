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

template <Callable<ALU&, Register2 &, Register2 &> C> auto aluTest(C&& callable) {
  Register2 flg = 0;
  Register2 ovf = 0;
  auto alu = ALU_ctor(&flg, &ovf);
  std::invoke(std::forward<C>(callable), alu, flg, ovf);
  ALU_dtor(alu);
}
}

TEST(AluTest, Init) {
  Register2 flg = 0;
  Register2 ovf = 0;
  ALU alu = ALU_ctor(&flg, &ovf);

  ALU_dtor(alu);
}

TEST(AluTest, ALU_add) {
  aluTest([](ALU& alu, Register2 & flg, Register2 & ovf) {
    Register2 p0 = 1;
    Register2 p1 = 2;
    auto instr = Instruction_ctor3(ALU_ADD, createRegister(&p0), createRegister(&p1));
    ALU_execute(alu, instr);
    Instruction_dtor(instr);
    ASSERT_EQ(3, p0);

  });
}

TEST(AluTest, ALU_sub) {
  aluTest([](ALU& alu, Register2 & flg, Register2 & ovf) {
    Register2 p0 = 2;
    Register2 p1 = 1;
    auto instr = Instruction_ctor3(ALU_SUB, createRegister(&p0), createRegister(&p1));
    ALU_execute(alu, instr);
    Instruction_dtor(instr);
    ASSERT_EQ(1, p0);

  });
}

TEST(AluTest, ALU_mul) {
  aluTest([](ALU& alu, Register2 & flg, Register2 & ovf) {
    Register2 p0 = 2;
    Register2 p1 = 2;
    auto instr = Instruction_ctor3(ALU_MUL, createRegister(&p0), createRegister(&p1));
    ALU_execute(alu, instr);
    Instruction_dtor(instr);
    ASSERT_EQ(4, p0);

  });
}

TEST(AluTest, ALU_div) {
  aluTest([](ALU& alu, Register2 & flg, Register2 & ovf) {
    Register2 p0 = 8;
    Register2 p1 = 2;
    auto instr = Instruction_ctor3(ALU_DIV, createRegister(&p0), createRegister(&p1));
    ALU_execute(alu, instr);
    Instruction_dtor(instr);
    ASSERT_EQ(4, p0);

  });
}

TEST(AluTest, ALU_div_with_remainder) {
  aluTest([](ALU& alu, Register2 & flg, Register2 & ovf) {
    Register2 p0 = 9;
    Register2 p1 = 2;
    auto instr = Instruction_ctor3(ALU_DIV, createRegister(&p0), createRegister(&p1));
    ALU_execute(alu, instr);
    Instruction_dtor(instr);
    ASSERT_EQ(4, p0);
    ASSERT_EQ(1, ovf);
  });
}

TEST(AluTest, ALU_zero_div_raise_flag) {
  aluTest([](ALU& alu, Register2 & flg, Register2 & ovf) {
    Register2 p0 = 9;
    Register2 p1 = 0;
    auto instr = Instruction_ctor3(ALU_DIV, createRegister(&p0), createRegister(&p1));
    ALU_execute(alu, instr);
    Instruction_dtor(instr);
    ASSERT_EQ(9, p0);
    ASSERT_EQ(FR_DIV_ZERO_FLAG, flg);
  });
}

TEST(AluTest, ALU_test_with_const_val) {
  aluTest([] (ALU& alu, Register2 & flg, Register2 & ovf) {
    Register2 p0 = 9;
    auto instr = Instruction_ctor3(ALU_ADD, createRegister(&p0), createConstant(4));
    ALU_execute(alu, instr);
    Instruction_dtor(instr);
    ASSERT_EQ(13, p0);
  });

}
