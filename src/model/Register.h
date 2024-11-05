//
// Created by rosa on 11/5/24.
//

#ifndef REGISTER_H
#define REGISTER_H

#include <Types.h>

typedef U16 Register;

static inline bool Register_isSet(Register actual, Register expected) {
  return (actual & expected) != 0;
}


#endif //REGISTER_H
