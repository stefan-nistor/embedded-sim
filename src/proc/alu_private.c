#include <Types.h>
#include <assert.h>
#include <model/Register.h>
#include <proc/Alu.h>
#include <stdlib.h>


typedef void (*OverflowConsumer)(Register2 *dst, U16 src);
typedef U32 (*BinaryOperator)(U16 lhs, U16 rhs);

typedef struct Private_ALU {

  Register2 *flagRegister;
  Register2 *overflowReg;

} Private_ALU;

#define DEFINE_OP(_name, _operand)                                                                                     \
  static U32 _name(U16 lhs, U16 rhs) { return (U32) ((U32) lhs _operand(U32) rhs); }

DEFINE_OP(sum, +)
DEFINE_OP(sub, -)
DEFINE_OP(mul, *)
DEFINE_OP(or, |)
DEFINE_OP(and, &)
DEFINE_OP(shl, <<)
DEFINE_OP(shr, >>)
DEFINE_OP(xor, ^)

static U32 not(U16 lhs, U16 rhs) { return (U32) ~lhs; }

static U32 div2(U16 lhs, U16 rhs) {
  U16 remainder = lhs % rhs;
  U16 result = lhs / rhs;

  return ((remainder << 16) & 0xFFFF0000) | (result & 0x0000FFFF);
}

static void compute(Private_ALU *self, BinaryOperator op, Register2 *lhs, Register2 *rhs, OverflowConsumer consumer) {
  U32 compoundResult = op(*lhs, *rhs);
  U16 result = compoundResult & 0xFFFFu;
  U16 overflow = (compoundResult >> 16) & 0xFFFFu;
  *lhs = result;
  consumer(self->overflowReg, overflow);
}

static void acceptOverflow(Register2 *dstReg, U16 src) { *dstReg = src; }

static void ignoreOverflow(Register2 *dstReg, U16 src) {
  (void) dstReg;
  (void) src;
}

Private_ALU *ALU_ctor(Register2 *reg, Register2 *overflowReg) {
  Private_ALU *alu = (Private_ALU *) malloc(sizeof(Private_ALU));
  alu->flagRegister = reg;
  alu->overflowReg = overflowReg;
  return alu;
}

static void ALU_add(Private_ALU *alu, Register2 *dstSrc0, Register2 *src1) {
  assert(src1 != NULL);
  compute(alu, sum, dstSrc0, src1, &ignoreOverflow);
}

static void ALU_sub(Private_ALU *alu, Register2 *dstSrc0, Register2 *src1) {
  assert(src1 != NULL);
  compute(alu, sub, dstSrc0, src1, &acceptOverflow);
}

static void ALU_mul(Private_ALU *alu, Register2 *dstSrc0, Register2 *src1) {
  assert(src1 != NULL);
  compute(alu, mul, dstSrc0, src1, &ignoreOverflow);
}

static void ALU_div(Private_ALU *alu, Register2 *dstSrc0, Register2 *src1) {
  if (*src1 == 0) {
    *alu->flagRegister |= FR_DIV_ZERO_FLAG;
    return;
  }

  compute(alu, div2, dstSrc0, src1, &acceptOverflow);
}

static void ALU_or(Private_ALU *alu, Register2 *dstScr0, Register2 *src1) {
  assert(src1 != NULL);
  compute(alu, or, dstScr0, src1, &ignoreOverflow);
}

static void ALU_and(Private_ALU *alu, Register2 *dstSrc0, Register2 *src1) {
  assert(src1 != NULL);
  compute(alu, and, dstSrc0, src1, &ignoreOverflow);
}

static void ALU_shl(Private_ALU *alu, Register2 *dstSrc0, Register2 *src1) {
  assert(src1 != NULL);
  compute(alu, shl, dstSrc0, src1, &ignoreOverflow);
}

static void ALU_shr(Private_ALU *alu, Register2 *dstSrc0, Register2 *src1) {
  assert(src1 != NULL);
  compute(alu, shr, dstSrc0, src1, &ignoreOverflow);
}

static void ALU_xor(Private_ALU *alu, Register2 *dstSrc0, Register2 *src1) {
  assert(src1 != NULL);
  compute(alu, xor, dstSrc0, src1, &ignoreOverflow);
}

static void ALU_not(Private_ALU *alu, Register2 *dstSrc0, Register2 *src1) {
  compute(alu, not, dstSrc0, src1, &ignoreOverflow);
}

static void ALU_cmp(Private_ALU *alu, const Register2 *src0, const Register2 *src1) {
  assert(src0 != NULL);
  assert(src1 != NULL);

  if (*src0 == *src1) {
    *alu->flagRegister |= FR_EQUAL_FLAG;
  } else if (*src0 < *src1) {
    *alu->flagRegister |= FR_LESS_FLAG;
  }
}

void ALU_execute(Private_ALU *self, Instruction instruction) {
  assert(instruction != NULL);
  assert(Instruction_getParam1(instruction) != NULL);
  assert(Instruction_getParam2(instruction) != NULL);

  Parameter p0 = Instruction_getParam1(instruction);
  Parameter p1 = Instruction_getParam2(instruction);

  assert(isParameterOf(p0, PT_REGISTER) && ILLEGAL_OPERATION);
  U16* pReg = getParameterRegister(p0);
  U16 val = getParameterValue(p1);

  switch (Instruction_getType(instruction)) {
    case ALU_ADD:
      ALU_add(self, pReg, &val);
      break;
    case ALU_SUB:
      ALU_sub(self, pReg, &val);
      break;
    case ALU_MUL:
      ALU_mul(self, pReg, &val);
      break;
    case ALU_DIV:
      ALU_div(self, pReg, &val);
      break;
    case ALU_OR:
      ALU_or(self, pReg, &val);
      break;
    case ALU_AND:
      ALU_and(self, pReg, &val);
      break;
    case ALU_XOR:
      ALU_xor(self, pReg, &val);
      break;
    case ALU_SHL:
      ALU_shl(self, pReg, &val);
      break;
    case ALU_SHR:
      ALU_shr(self, pReg, &val);
      break;
    case ALU_NOT:
      ALU_not(self, pReg, &val);
      break;
    case ALU_CMP:
      ALU_cmp(self, pReg, &val);
      break;
    default:
      assert(false && INVALID_INSTR);
  }

  assert(!Register_isSet(*self->flagRegister, FR_ILLEGAL_FLAG) &&
         !Register_isSet(*self->flagRegister, FR_MULTISTATE_FLAG) &&
         !Register_isSet(*self->flagRegister, FR_SEG_FLAG)&&
         "Unexpected error raised");
}

void ALU_dtor(Private_ALU *mem) { free(mem); }
