//
// Created by loghin on 11/8/24.
//

#pragma once

#include <cassert>
#include <utility>

#include <model/Instruction.h>

namespace cxx::detail {
using std::exchange;

class Instruction {
public:
  explicit Instruction(InstructionType type = DEFAULT, Register2 * r0 = nullptr, Register2 * r1 = nullptr) :
      _instr{Instruction_ctor3(type, r0, r1)} {
    assert(_instr && "Instruction constructor yielded null memory");
  }

  Instruction(Instruction const&) = delete;
  Instruction(Instruction&& instr) noexcept : _instr{exchange(instr._instr, nullptr)} {}
  auto operator=(Instruction const&) -> Instruction& = delete;
  auto operator=(Instruction&& instr) noexcept -> Instruction& {
    if (this == &instr) {
      return *this;
    }

    Instruction_dtor(exchange(_instr, exchange(instr._instr, nullptr)));
    return *this;
  };

  ~Instruction() noexcept {
    Instruction_dtor(_instr);
  }

  [[nodiscard]] constexpr auto handle() const noexcept {
    return _instr;
  }

  constexpr explicit operator ::Instruction() const noexcept {
    return handle();
  }

private:
  ::Instruction _instr {nullptr};
};
} // namespace cxx::detail

namespace cxx {
using detail::Instruction;
} // namespace cxx
