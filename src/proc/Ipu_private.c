//
// Created by rosa on 11/5/24.
//

#include "model/Instruction.h"
#include "model/Register.h"
typedef struct Private_IPU {

  Register *flagRegister;
  Register* programCounter;
  Instruction instructions[];


} Private_IPU;