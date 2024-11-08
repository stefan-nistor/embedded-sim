//
// Created by loghin on 11/8/24.
//

#pragma once

#include <bit>
#include <iostream>
#include <optional>
#include <sstream>
#include <type_traits>
#include <variant>
#include <vector>

#include <generic/cxx/RAII.hpp>

namespace testing::mock::detail {
using std::apply;
using std::bit_cast;
using std::cout;
using std::equal;
using std::forward_as_tuple;
using std::hex;
using std::holds_alternative;
using std::is_same_v;
using std::optional;
using std::nullopt;
using std::nullptr_t;
using std::remove_cvref_t;
using std::size_t;
using std::string;
using std::stringstream;
using std::to_string;
using std::variant;
using std::vector;

struct Wildcard{};

enum class ParameterMatchResult {
  ParamEqual,
  NullParamMismatch,
  ValueMismatch,
  RegisterMismatch,
};

enum class InstructionMatchResult {
  InstrEqual,
  TypeMismatch,
  P0Mismatch,
  P1Mismatch,
};

using ParameterMatcherTypes = variant<int, nullptr_t, Register const*>;

template <typename T> struct WithWildcardImpl;
template <typename...Ts> struct WithWildcardImpl<variant<Ts...>> {
  using Type = variant<Wildcard, Ts...>;
};

template <typename T> using WithWildcard = typename WithWildcardImpl<T>::Type;

template <typename T, typename... R> auto getOrNullopt(variant<R...> const& v) -> optional<T> {
  if (holds_alternative<T>(v)) {
    return get<T>(v);
  }
  return nullopt;
}

class ParameterMatcher {
public:
  explicit ParameterMatcher(WithWildcard<ParameterMatcherTypes> const& param = Wildcard{}) :
      _param {std::visit([]<typename T>(T&& arg) -> optional<ParameterMatcherTypes> {
        if constexpr (is_same_v<remove_cvref_t<T>, Wildcard>) {
          return nullopt;
        } else {
          return arg;
        }
      }, param)} {}

  auto match(Register const* param) const noexcept {
    using enum ParameterMatchResult;
    if (!_param) {
      return ParamEqual;
    }

    if (holds_alternative<nullptr_t>(*_param) == static_cast<bool>(param)) {
      return NullParamMismatch;
    }

    if (holds_alternative<nullptr_t>(*_param) && !param) {
      return ParamEqual;
    }

    assert(param && "Param should be not null at this point");
    if (holds_alternative<int>(*_param) && *param != get<int>(*_param)) {
      return ValueMismatch;
    }

    if (holds_alternative<Register const*>(*_param) && param != get<Register const*>(*_param)) {
      return RegisterMismatch;
    }

    return ParamEqual;
  }

  [[nodiscard]] auto str() const {
    assert(_param && "Wildcard dump unexpected");
    return std::visit([]<typename DT>(DT&& val) -> string {
      using T = remove_cvref_t<DT>;
      if constexpr(is_same_v<nullptr_t, T>) {
        return "null";
      } else if constexpr(is_same_v<int, T>) {
        return to_string(val);
      } else if constexpr(is_same_v<Register const*, T>) {
        stringstream oss;
        oss << *val << " (" << "0x" << hex << bit_cast<size_t>(val) << ")";
        return oss.str();
      } else {
        assert(false && "Unhandled ParameterMatcher str alternative");
      }
    }, *_param);
  }

private:
  optional<ParameterMatcherTypes> _param {nullopt};
};

class InstructionMatcher {
public:
  explicit InstructionMatcher(
      variant<Wildcard, InstructionType> const& type = Wildcard{},
      WithWildcard<ParameterMatcherTypes> const& p0 = Wildcard{},
      WithWildcard<ParameterMatcherTypes> const& p1 = Wildcard{}
  ) : _type{getOrNullopt<InstructionType>(type)},
      _p0{p0},
      _p1{p1} {}

  [[nodiscard]] auto const& type() const { return _type; }
  [[nodiscard]] auto const& p0() const { return _p0; }
  [[nodiscard]] auto const& p1() const { return _p1; }

  auto match(Instruction instruction) const {
    using enum InstructionMatchResult;
    using enum ParameterMatchResult;
    if (_type && *_type != Instruction_getType(instruction)) {
      return TypeMismatch;
    }

    if (auto match = _p0.match(Instruction_getParam1(instruction)); match != ParamEqual) {
      return P0Mismatch;
    }

    if (auto match = _p1.match(Instruction_getParam2(instruction)); match != ParamEqual) {
      return P1Mismatch;
    }

    return InstrEqual;
  }

private:
  optional<InstructionType> _type {nullopt};
  ParameterMatcher _p0;
  ParameterMatcher _p1;
};

class InstructionMatcherSet {
public:
  [[nodiscard]] auto const& matchers() const {
    return _matchers;
  }

  auto push(InstructionMatcher&& instr) {
    _matchers.push_back(std::move(instr));
  }

  auto push(Wildcard) {
    _matchers.emplace_back();
  }

  template <typename... Ts> auto push(tuple<Ts...>&& tArgs) {
    apply([this](Ts&&... args) {
      _matchers.emplace_back(Wildcard{}, std::forward<Ts>(args)...);
    }, std::move(tArgs));
  }

private:
  vector<InstructionMatcher> _matchers;
};

template <typename... A> auto instructions(A&&... args) -> InstructionMatcherSet {
  InstructionMatcherSet set;
  (set.push(std::forward<A>(args)), ...);
  return set;
}

auto any() {
  return Wildcard{};
}

template <typename... A> auto any(A&&... args) {
  return forward_as_tuple(args...);
}

template <typename... A> auto instr(A&&... args) {
  return InstructionMatcher{std::forward<A>(args)...};
}

template <typename... A> auto mov(A&&... args) {
  return instr(MMU_MOV, std::forward<A>(args)...);
}

template <typename... A> auto add(A&&... args) {
  return instr(ALU_ADD, std::forward<A>(args)...);
}

template <typename... A> auto sub(A&&... args) {
  return instr(ALU_SUB, std::forward<A>(args)...);
}

template <typename... A> auto mul(A&&... args) {
  return instr(ALU_MUL, std::forward<A>(args)...);
}

template <typename... A> auto jmp(A&&... args) {
  return instr(IPU_JMP, std::forward<A>(args)...);
}

template <typename... A> auto pop(A&&... args) {
  return instr(MMU_POP, std::forward<A>(args)...);
}

auto str(InstructionType type) {
  switch(type) {
    case DEFAULT:   return "<<default>>";
    case ALU_ADD:   return "add";
    case ALU_SUB:   return "sub";
    case ALU_MUL:   return "mul";
    case ALU_DIV:   return "div";
    case ALU_AND:   return "and";
    case ALU_OR:    return "or";
    case ALU_XOR:   return "xor";
    case ALU_NOT:   return "not";
    case ALU_SHL:   return "shl";
    case ALU_SHR:   return "shr";
    case ALU_CMP:   return "cmp";
    case IPU_JMP:   return "jmp";
    case IPU_JEQ:   return "jeq";
    case IPU_JNE:   return "jne";
    case IPU_JLT:   return "jlt";
    case IPU_JLE:   return "jle";
    case IPU_JGT:   return "jgt";
    case IPU_JGE:   return "jge";
    case IPU_CALL:  return "call";
    case IPU_RET:   return "ret";
    case MMU_MOV:   return "mov";
    case MMU_PUSH:  return "push";
    case MMU_POP:   return "pop";
    default:
      assert(false && "Unhandled InstructionType str");
  }
}

auto str(ParameterMatcher const& matcher) {
  return matcher.str();
}

auto str(Register const* r) -> string {
  if (!r) {
    return "null";
  }

  stringstream oss;
  oss << *r << " (0x" << hex << bit_cast<size_t>(r) << ")";
  return oss.str();
}

auto operator==(InstructionMatcherSet const& instructionSet, vector<Instruction> const& instructions) {
  using enum InstructionMatchResult;
  using enum ParameterMatchResult;
  auto const& r1 = instructionSet.matchers();
  auto const& r2 = instructions;
  auto b1 = r1.begin();
  auto e1 = r1.end();
  auto b2 = r2.begin();
  auto e2 = r2.end();

  for (; b1 != e1 && b2 != e2; ++b1, ++b2) {
    if (auto match = b1->match(*b2); match != InstrEqual) {
      cout << "[Test - InstructionMatch] Failure: Instruction '" << b1 - r1.begin() << "' mismatch:\n";
      [match, &expected = *b1, actual = *b2]() {
        switch (match) {
          case TypeMismatch:
            cout << "\tType Mismatch: Expected='" << str(*expected.type())
                 << "', Actual='" << str(Instruction_getType(actual)) << "'\n";
            break;
          case P0Mismatch:
          case P1Mismatch: {
            cout << "\tParameter " << (match == P0Mismatch ? 0 : 1) << " Mismatch.\n";
            auto const& matcher = (match == P0Mismatch ? expected.p0() : expected.p1());
            auto const param = (match == P0Mismatch ? Instruction_getParam1(actual) : Instruction_getParam2(actual));
            cout << "\t\tExpected='" << str(matcher) << "', Actual='" << str(param) << "'\n";
            break;
          }
          default:
            assert(false && "Unhandled InstructionMatchResult");
        }
      }();
      return false;
    }
  }

  if (b1 != e1) {
    cout << "[Test - InstructionMatch] Failure: Expected '" << e1 - b1 << "' more instruction(s)\n";
    return false;
  }

  if (b2 != e2) {
    cout << "[Test - InstructionMatch] Failure: Expected '" << e2 - b2 << "' less instruction(s)\n";
    return false;
  }

  return true;
}
} // namespace testing::mock::detail

namespace testing::mock {
using detail::instructions;

using detail::add;
using detail::any;
using detail::jmp;
using detail::mov;
using detail::mul;
using detail::pop;
using detail::sub;
} // namespace testing::mock
