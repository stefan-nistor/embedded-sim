// Created by logout

#include "parser.h"

#include <cassert>
#include <functional>
#include <filesystem>
#include <format>
#include <fstream>
#include <memory>
#include <numeric>
#include <iostream>
#include <optional>
#include <string>
#include <sstream>
#include <stdexcept>
#include <tuple>
#include <utility>
#include <unordered_map>
#include <variant>
#include <vector>

#include <model/Instruction.h>
#include <model/InstructionType.h>
#include <model/Register.h>

namespace {
using std::iota;
using std::addressof;
using std::cerr;
using std::char_traits;
using std::construct_at;
using std::destroy_at;
using std::exception;
using std::exchange;
using std::format;
using std::holds_alternative;
using std::getline;
using std::fstream;
using std::ios;
using std::invoke;
using std::ignore;
using std::nullopt;
using std::optional;
using std::runtime_error;
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

static_assert(sizeof(InstructionRAII) == sizeof(Instruction));

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
  Full,
  Accepted,
  AcceptedFinished,
};

using Reference = string;
using Constant = unsigned;
using Parameter = variant<Reference, Constant>;
using Instr = tuple<InstructionType, optional<Parameter>, optional<Parameter>>;
using Label = string;

class EncodedInstruction {
public:
  explicit EncodedInstruction(InstructionType type, unsigned idx) noexcept :
      _idx{idx}, _encoded{Instr{type, nullopt, nullopt}} {}
  explicit EncodedInstruction(string_view sv, unsigned refInstrIdx) noexcept :
      _idx{refInstrIdx}, _encoded{Label{sv}} {}

  auto feed(string_view sv) -> FeedResult {
    if (sv == ":" || sv == ";") {
      return AcceptedFinished;
    }

    assert(holds_alternative<Instr>(_encoded) && "Unexpected parametrized label");
    auto& [t, p0, p1] = get<Instr>(_encoded);
    auto const [minParamCount, maxParamCount] = instructionOpCount(t);
    auto const paramCount = currentParameterCount();
    if (paramCount == maxParamCount) {
      return Full;
    }
    addParam(makeParam(sv));
    if (paramCount == maxParamCount) {
      return AcceptedFinished;
    }

    return Accepted;
  }

  [[nodiscard]] auto index() const noexcept {
    return _idx;
  }

  template <typename IfInstr, typename IfLabel> [[nodiscard]]
  decltype(auto) visit(IfInstr&& ifInstr, IfLabel&& ifLabel) noexcept {
    return std::visit([this, ifLabel, &ifInstr]<typename DT>(DT&& val) {
      using T = std::remove_cvref_t<DT>;
      if constexpr (std::is_same_v<T, Instr>) {
        auto&& [type, p0, p1] = val;
        return invoke(ifInstr, type, std::move(p0), std::move(p1));
      } else if constexpr (std::is_same_v<T, Label>) {
        return invoke(ifLabel, std::move(std::forward<DT>(val)), _idx);
      } else {
        assert(false && "Unhandled EncodedInstruction alternative");
        terminate();
      }
    }, _encoded);
  }

private:
  static auto makeConstantParam(string_view sv) -> Constant {
    if (sv == "0") {
      return 0;
    }

    char* end = nullptr;
    auto value = [sv, &end]() {
      if (sv.front() == '0') {
        if (sv[1] == 'b' || sv[1] == 'B') {
          return strtol(sv.data() + 2, &end, 2);
        } else if (sv[1] == 'x' || sv[1] == 'X') {
          return strtol(sv.data() + 2, &end, 16);
        } else {
          return strtol(sv.data() + 2, &end, 8);
        }
      }
      return strtol(sv.data(), &end, 10);
    }();

    if (end < sv.data() + sv.length()) {
      throw runtime_error(format("Invalid token: '{}'", sv));
    }
    return value;
  }

  static auto makeReferenceParam(string_view sv) -> Reference {
    return Reference{sv};
  }

  static auto makeParam(string_view sv) -> Parameter {
    if ('0' <= sv[0] && sv[0] <= '9') {
      return makeConstantParam(sv);
    }
    return makeReferenceParam(sv);
  }

  [[nodiscard]] auto currentParameterCount() const -> unsigned {
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

  auto addParam(Parameter const& p) -> void {
    assert(holds_alternative<Instr>(_encoded) && "Invalid instruction encoding");
    auto& [_, p0, p1] = get<Instr>(_encoded);
    if (!p0) {
      p0 = p;
    } else {
      p1 = p;
    }
  }

  variant<Label, Instr> _encoded;
  unsigned _idx;
};

class Tokenizer {
public:
  auto feed(string_view token) -> optional<EncodedInstruction> {
    if (_lineComment || !_current && token == "//") {
      _lineComment = true;
      return nullopt;
    }

    if (token == ",") {
      return nullopt;
    }

    if (!_current) {
      if (auto const opToken = op(token)) {
        _current.emplace(*opToken, ++_instructionIndex);
      } else {
        return EncodedInstruction{token, _instructionIndex};
      }
    } else {
      auto res = _current->feed(token);
      switch (res) {
        case AcceptedFinished:
        case Full: {
          auto finished = std::move(*_current);
          _current.reset();
          if (res == Full) {
            ignore = feed(token);
          }
          return finished;
        }
        case Accepted:
          return nullopt;
      }
    }

    return nullopt;
  }

  auto newLine() {
    _lineComment = false;
  }

  auto anyRemaining() noexcept -> optional<EncodedInstruction> {
    return _current;
  }

private:
  bool _lineComment {false};
  unsigned _instructionIndex {0};
  optional<EncodedInstruction> _current {nullopt};
};

template <typename T> auto revive(T&& object) {
  destroy_at(addressof(std::forward<T>(object)));
  construct_at(addressof(object));
}

auto sanitize(string_view sv) {
  if (sv.empty()) {
    return sv;
  }
  if (sv.back() == ';') {
    return sv.substr(0, sv.length() - 1);
  }
  return sv;
}

class CxxParser {
public:
  explicit CxxParser(string&& code) : _code{std::move(code)} {
    _possibleConstants.resize(std::numeric_limits<Register>::max());
    std::iota(_possibleConstants.begin(), _possibleConstants.end(), 0);
    Tokenizer tokenizer;
    string line;
    while (getline(_code, line)) {
      tokenizer.newLine();
      stringstream lineBuf {std::move(line)};
      revive(line);
      string token;
      while (lineBuf >> token) {
        auto const sanitized = sanitize(token);
        if (auto maybeInstruction = tokenizer.feed(sanitized)) {
          _encodedInstructions.push_back(std::move(*maybeInstruction));
        }
      }
    }

    if (auto maybeInstruction = tokenizer.anyRemaining()) {
      _encodedInstructions.push_back(std::move(*maybeInstruction));
    }
  }

  auto makeInstructionSet(/* CPU */) noexcept -> vector<InstructionRAII> const& {
    // TODO: associate _cachedInstructions with CPU to regenerate for different received map

    if (_cachedInstructions) {
      return *_cachedInstructions;
    }

    _cachedInstructions.emplace();
    _cachedInstructions->reserve(_encodedInstructions.size());
    unordered_map<string_view, unsigned> jumpMap;
    for (auto& encoded : _encodedInstructions) {
      encoded.visit(
          [](auto&&...) {},
          [&jumpMap, this](auto const& label, unsigned instrRefIdx) {
            jumpMap.emplace(label, instrRefIdx);
          }
      );
    }

    for (auto&& encoded : std::move(_encodedInstructions)) {
      encoded.visit(
          [this, &jumpMap](InstructionType type, optional<Parameter>&& p0, optional<Parameter>&& p1) {
            auto paramVisitor = [this, &jumpMap](optional<Parameter>&& p) -> Register* {
              if (!p) {
                return nullptr;
              }

              return std::visit([this, &jumpMap]<typename DT>(DT&& val) -> Register* {
                using T = std::remove_cvref_t<DT>;
                if constexpr (std::is_same_v<T, Reference>) {
                  if (auto jumpAt = jumpMap.find(val); jumpAt != jumpMap.end()) {
                    return _possibleConstants.data() + jumpAt->second;
                  } else {
                    // Register reference, identify in CPU, or unmapped/invalid reference
                    return nullptr;
                  }
                } else if constexpr (std::is_same_v<T, Constant>) {
                  return _possibleConstants.data() + val;
                } else {
                  assert(false && "Unhandled Parameter type");
                  return nullptr;
                }
              }, std::move(*p));
            };
            _cachedInstructions->emplace_back(type, paramVisitor(std::move(p0)), paramVisitor(std::move(p1)));
          },
          [](auto&&...) {}
      );
    }

    _cachedInstructions->shrink_to_fit();
    return *_cachedInstructions;
  }

private:
  stringstream _code;
  vector<EncodedInstruction> _encodedInstructions;
  optional<vector<InstructionRAII>> _cachedInstructions;
  vector<Register> _possibleConstants;
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

void Parser_destruct(Parser parser) {
  delete parser;
}

U16 Parser_getInstructionSet(Parser parser, Instruction const** ppInstructionList /*, CPUMap? */) {
  auto const& instructions = parser->parser.makeInstructionSet();
  if (ppInstructionList) {
    static_assert(sizeof(InstructionRAII) == sizeof(Instruction));
    static_assert(sizeof(instructions[0]) == sizeof(Instruction));
    *ppInstructionList = static_cast<Instruction const*>(static_cast<void const*>(instructions.data()));
  }
  return static_cast<U16>(instructions.size());
}
}
