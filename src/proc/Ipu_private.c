//
// Created by rosa on 11/5/24.
//

#include "model/Instruction.h"
#include "model/Register.h"
typedef struct Private_IPU {

  Register2 *flagRegister;
  Register2 * programCounter;
  Instruction instructions[];


} Private_IPU;