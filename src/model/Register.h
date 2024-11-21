//
// Created by rosa on 11/5/24.
//

#ifndef REGISTER_H
#define REGISTER_H

#include <Types.h>

typedef U16 Register2;

static inline bool Register_isSet(Register2 actual, Register2 expected) {
  return (actual & expected) != 0;
}


#endif //REGISTER_H
