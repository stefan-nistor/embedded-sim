// Created by logout

#include "parser.h"

#include <cassert>
#include <functional>
#include <filesystem>
#include <fstream>
#include <memory>
#include <iostream>
#include <optional>
#include <string>
#include <sstream>
#include <tuple>
#include <utility>
#include <unordered_map>
#include <variant>
#include <vector>

#include <model/Instruction.h>
#include <model/InstructionType.h>
#include <model/Register.h>

namespace {
using std::addressof;
using std::cerr;
using std::char_traits;
using std::construct_at;
using std::destroy_at;
using std::exception;
using std::exchange;
using std::holds_alternative;
using std::getline;
using std::fstream;
using std::ios;
using std::invoke;
using std::nullopt;
using std::optional;
using std::string;
using std::stringstream;
using std::string_view;
using std::terminate;
using std::tuple;
using std::unordered_map;
using std::variant;
using std::vector;
using std::visit;

namespace fs = std::filesystem;

class InstructionRAII {
public:
  explicit InstructionRAII(InstructionType type = DEFAULT, Register* r0 = nullptr, Register* r1 = nullptr) :
      _instr{Instruction_ctor3(type, r0, r1)} {
    assert(_instr && "Instruction constructor yielded null memory");
  }

  InstructionRAII(InstructionRAII const&) = delete;
  InstructionRAII(InstructionRAII&& instr) noexcept : _instr{exchange(instr._instr, nullptr)} {}
  auto operator=(InstructionRAII const&) -> InstructionRAII& = delete;
  auto operator=(InstructionRAII&& instr) noexcept -> InstructionRAII& {
    if (this == &instr) {
      return *this;
    }

    Instruction_dtor(exchange(_instr, exchange(instr._instr, nullptr)));
    return *this;
  };

  ~InstructionRAII() noexcept {
    Instruction_dtor(_instr);
  }

  [[nodiscard]] constexpr auto handle() const noexcept {
    return _instr;
  }

  constexpr explicit(false) operator Instruction() const noexcept {
    return handle();
  }

private:
  Instruction _instr {nullptr};
};

auto instructionOpCount(InstructionType type) noexcept -> tuple<unsigned, unsigned> {
  switch (type) {
    case ALU_ADD:
    case ALU_SUB:
    case ALU_MUL:
    case ALU_DIV:
    case ALU_AND:
    case ALU_OR:
    case ALU_XOR:
    case ALU_SHL:
    case ALU_SHR:
    case ALU_CMP:
    case MMU_MOV:
      return {2, 2};
    case ALU_NOT:
    case IPU_JMP:
    case IPU_JEQ:
    case IPU_JNE:
    case IPU_JLT:
    case IPU_JLE:
    case IPU_JGT:
    case IPU_JGE:
    case IPU_CALL:
    case MMU_PUSH:
      return {1, 1};
    case IPU_RET:
      return {0, 0};
    case MMU_POP:
      return {0, 1};
    default:
      assert(false && "Unhandled instruction type case");
      terminate();
  }
}

unordered_map<string_view, InstructionType> const iTypeMap {
    {"add", ALU_ADD},
    {"sub", ALU_SUB},
    {"mul", ALU_MUL},
    {"div", ALU_DIV},
    {"and", ALU_AND},
    {"or", ALU_OR},
    {"xor", ALU_XOR},
    {"not", ALU_NOT},
    {"shl", ALU_SHL},
    {"shr", ALU_SHR},
    {"cmp", ALU_CMP},
    {"jmp", IPU_JMP},
    {"jeq", IPU_JEQ},
    {"jne", IPU_JNE},
    {"jlt", IPU_JLT},
    {"jle", IPU_JLE},
    {"jgt", IPU_JGT},
    {"jge", IPU_JGE},
    {"call",IPU_CALL},
    {"ret", IPU_RET},
    {"mov", MMU_MOV},
    {"push",MMU_PUSH},
    {"pop", MMU_POP}
};

auto op(string_view token) -> optional<InstructionType> {
  if (auto const it = iTypeMap.find(token); it != iTypeMap.end()) {
    return it->second;
  }
  return nullopt;
}

enum FeedResult {
  Accepted,
  AcceptedFinished,
  NotFinished
};

class EncodedInstruction {
public:
  explicit EncodedInstruction(InstructionType type) noexcept : _encoded{Instr{type, nullopt, nullopt}} {}
  explicit EncodedInstruction(string_view sv) noexcept : _encoded{Label{sv}} {}

  auto feed(string_view sv) noexcept -> FeedResult {
    assert(holds_alternative<Label>(_encoded) && "Unexpected parametrized label");

//    return visit([this, sv]<typename DT>(DT&& val) {
//      using T = std::decay_t<DT>;
//      if constexpr (std::is_same_v<T, Label>) {
//        assert(false && "Unexpected parametrized label");
//        terminate();
//      } else if constexpr (std::is_same_v<T, Instr>) {
//        auto& [t, p0, p1] = val;
//        auto const [minParamCount, maxParamCount] = instructionOpCount(t);
//        if (currentParameterCount() < maxParamCount) {
//          addParam(makeParam(sv));
//        }
//
//        if (auto count = currentParameterCount() == maxParamCount) {
//          return AcceptedFinished;
//        } else if (count == minParamCount) {
//          return Accepted;
//        }
//        return NotFinished;
//      } else {
//        assert(false && "Unhandled variant alternative");
//        terminate();
//      }
//    });
  }

private:
  using Reference = string;
  using Constant = unsigned;
  using Parameter = variant<Reference, Constant>;
  using Instr = tuple<InstructionType, optional<Parameter>, optional<Parameter>>;

  static auto makeParam(string_view sv) -> Parameter {

  }

  auto currentParameterCount() const -> unsigned {
    assert(holds_alternative<Instr>(_encoded) && "Invalid instruction encoding");
    auto const& [_, p0, p1] = get<Instr>(_encoded);
    if (p0 && p1) {
      return 2;
    }
    if (p0) {
      return 1;
    }
    return 0;
  }

  auto addParam(Parameter p) -> void {
    assert(holds_alternative<Instr>(_encoded) && "Invalid instruction encoding");
    auto& [_, p0, p1] = get<Instr>(_encoded);
    if (!p0) {
      p0 = p;
    } else {
      p1 = p;
    }
  }

  using Label = string;

  variant<Label, Instr> _encoded;
};

class Tokenizer {
public:
  auto feed(string_view token) noexcept -> optional<EncodedInstruction> {
    if (_lineComment || !_current && token == "//") {
      _lineComment = true;
      return nullopt;
    }

    if (!_current) {
      if (auto const opToken = op(token)) {
        _current.emplace(*opToken);
      } else {
        return EncodedInstruction{token};
      }
    } else {
      auto fed = _current->feed(token);
      if (!fed) {
        auto extracted =
      }
      auto currentInstr = *_current;
      if (currentInstr.feed(token)) {

      }
    }


    else if (_current->feed(token)) {
      return std::move(*_current);
      _current.reset();
    } else {
      auto current = std::move(*_current);
      _current.reset();
      _current = feed(token);
      return current;
    }

    return nullopt;
  }

  auto newLine() {
    _lineComment = false;
  }

private:
  bool _lineComment;
  optional<EncodedInstruction> _current;
};

template <typename T> auto revive(T&& object) {
  destroy_at(addressof(std::forward<T>(object)));
  construct_at(addressof(object));
}

class CxxParser {
public:
  explicit CxxParser(string&& code) noexcept : _code{std::move(code)} {
    Tokenizer tokenizer;
    string line;
    while (getline(_code, line)) {
      tokenizer.newLine();
      stringstream lineBuf {std::move(line)};
      revive(line);
      string token;
      while (lineBuf >> token) {
        if (auto const maybeInstruction = tokenizer.feed(token)) {
          _instructions.push_back(std::move(*maybeInstruction));
        }
      }
    }
  }

private:
  stringstream _code;
  unordered_map<string, EncodedInstruction const*> _jumpMap;
  vector<EncodedInstruction> _instructions;
};

template <typename Callable> decltype(auto) catchAll(Callable&& callable) noexcept {
  try {
    return invoke(std::forward<Callable>(callable));
  } catch (exception const& e) {
    cerr << "Unhandled exception escaping to C code: " << e.what();
    terminate();
  }
}

auto parsePath(string_view path) -> string {
  if (!fs::exists(path)) {
    cerr << "Invalid input path '" << path << "'\n";
    return "";
  }

  fstream file{path.data(), ios::in};
  stringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}
} // namespace

extern "C" {
typedef struct Parser_Handle {
  CxxParser parser;
} Parser_Handle;

Parser Parser_fromPath(char const* path) {
  return catchAll([path](){
    auto code = parsePath(path);
    return new Parser_Handle{.parser{std::move(code)}};
  });
}

Parser Parser_fromCode(char const* code) {
  return catchAll([code]() {
    return Parser_fromSizedCode(char_traits<char>::length(code), code);
  });
}

Parser Parser_fromSizedCode(size_t length, char const* code) {
  return catchAll([length, code]() { return new Parser_Handle{.parser{string{code, length}}}; });
}

void Parser_dtor(Parser parser) {
  delete parser;
}
}
