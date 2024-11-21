//
// Created by rosa on 11/22/24.
//
#include <assert.h>
#include <model/parameter/Parameter.h>
#include <stdlib.h>

typedef struct {
  U16 value;
} ParameterConstant;

typedef struct {
  U16 value;
} ParameterMemoryLocation;

typedef struct {
  U16 *pValue;
} ParameterRegister;

struct Parameter_T {
  ParameterType type;

  union {
    ParameterConstant constant;
    ParameterMemoryLocation memory_location;
    ParameterRegister _register;
  };
};

inline static Parameter createParameter(ParameterType type) {
  const Parameter obj = malloc(sizeof(struct Parameter_T));
  obj->type = type;
  return obj;
}

Parameter createConstant(U16 value) {
  Parameter obj = createParameter(PT_CONSTANT);
  obj->constant.value = value;
  return obj;
}

Parameter createRegister(U16 * pReg) {
  Parameter obj = createParameter(PT_REGISTER);
  obj->_register.pValue = pReg;
  return obj;
}

Parameter createMemoryLocation(U16 pMemLoc) {
  Parameter obj = createParameter(PT_MEMORY_LOCATION);
  obj->memory_location.value = pMemLoc;
  return obj;
}

void destroyParameter(Parameter parameter) {
  free(parameter);
}

bool isParameterOf(Parameter param, ParameterType expected) {
  return expected == param->type;
}

U16 getParameterValue(Parameter parameter) {
  switch (parameter->type) {
    case PT_REGISTER:
      return *parameter->_register.pValue;
    case PT_CONSTANT:
      return parameter->constant.value;
    case PT_MEMORY_LOCATION:
      assert(false && ILLEGAL_OPERATION);
      break;
  }
}

U16* getParameterRegister(Parameter parameter) {
  assert(parameter->type == PT_REGISTER && ILLEGAL_OPERATION);
  return parameter->_register.pValue;
}