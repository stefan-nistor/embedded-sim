//
// Created by rosa on 11/22/24.
//

#ifndef PARAMETER_H
#define PARAMETER_H
#include <Types.h>

typedef struct Parameter_T *Parameter;

Parameter createConstant(U16 val);
Parameter createMemoryLocation(U16 pMemLoc);
Parameter createRegister(U16 *pReg);
void destroyParameter(Parameter);

bool isParameterOf(Parameter param, ParameterType expected);
void setParameterValue(Parameter param, U16 value);

U16 getParameterValue(Parameter parameter);
U16* getParameterRegister(Parameter parameter);

#endif //PARAMETER_H
