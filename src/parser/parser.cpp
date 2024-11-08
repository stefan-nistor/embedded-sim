// Created by logout

#include "parser.h"

#include <cassert>
#include <functional>
#include <filesystem>
#include <fstream>
#include <memory>
#include <numeric>
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

#include <generic/cxx/StructureTypeUtils.hpp>
#include <generic/cxx/RAII.hpp>

namespace {
using std::iota;
using std::addressof;
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
using std::ignore;
using std::is_same_v;
using std::nullopt;
using std::numeric_limits;
using std::optional;
using std::remove_cvref_t;
using std::string;
using std::stringstream;
using std::string_view;
using std::terminate;
using std::tuple;
using std::unordered_map;
using std::variant;
using std::vector;

namespace fs = std::filesystem;

using Reference = string;
using Constant = unsigned;
using Parameter = variant<Reference, Constant>;
using Instr = tuple<InstructionType, optional<Parameter>, optional<Parameter>>;
using Label = string;

enum class FeedResult {
  Full,
  Accepted,
  AcceptedFinished,
};

class NoMessageException : public exception {
  [[nodiscard]] auto what() const noexcept -> char const* override {
    return "";
  }
};

class InvalidPathException : public NoMessageException {};

class InvalidTokenException : public exception {
public:
  explicit InvalidTokenException(string_view token, unsigned lOffset = 0, unsigned cOffset = 0) :
      _token{token}, _lOffset{lOffset}, _cOffset{cOffset} {}
  InvalidTokenException(InvalidTokenException const& e) : _token{e._token} {}

  [[nodiscard]] auto what() const noexcept -> char const* override {
    return _token.c_str();
  }

  [[nodiscard]] auto token() const noexcept -> string_view {
    return _token;
  }

  [[nodiscard]] auto lineOffset() const noexcept {
    return _lOffset;
  }

  [[nodiscard]] auto columnOffset() const noexcept {
    return _cOffset;
  }

private:
  string _token;
  unsigned _lOffset {0};
  unsigned _cOffset {0};
};

class LocatedInvalidTokenException : public InvalidTokenException {
public:
  LocatedInvalidTokenException(InvalidTokenException const& tokenException, unsigned line, unsigned column) :
      InvalidTokenException{tokenException},
      _line{line + tokenException.lineOffset()}, _column{column + tokenException.columnOffset()} {}

  [[nodiscard]] auto line() const noexcept {
    return _line;
  }

  [[nodiscard]] auto column() const noexcept {
    return _column;
  }

private:
  unsigned _line;
  unsigned _column;
};

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
    {"push", MMU_PUSH},
    {"pop", MMU_POP}
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
    case MMU_POP:
      return {0, 1};
    case IPU_RET:
      return {0, 0};
    default:
      assert(false && "Unhandled instruction type case");
      terminate();
  }
}

auto op(string_view token) noexcept -> optional<InstructionType> {
  if (auto const it = iTypeMap.find(token);
      it != iTypeMap.end()) {
    return it->second;
  }
  return nullopt;
}

template <typename T> auto revive(T&& object) {
  destroy_at(addressof(std::forward<T>(object)));
  construct_at(addressof(object));
}

auto sanitize(string_view sv) {
  if (sv.empty()) {
    return sv;
  }
  if (sv.back() == ';' || sv.back() == ',') {
    sv.remove_suffix(1);
  }
  return sv;
}

auto parsePath(string_view path) -> string {
  if (!fs::exists(path)) {
    throw InvalidPathException();
  }

  fstream file{path.data(), ios::in};
  stringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

auto validateLabel(string_view token) {
  auto inRange = [](auto b, auto e, auto t) {
    return b <= t && t <= e;
  };
  if (token.empty()) {
    throw InvalidTokenException("");
  }
  if (!inRange('a', 'z', token.front())
      && !inRange('A', 'Z', token.front())
      && '_' != token.front()) {
    throw InvalidTokenException(token);
  }
  if (!token.ends_with(':')) {
    throw InvalidTokenException(token);
  }

  token.remove_suffix(1);
  return token;
}

class EncodedInstruction {
public:
  explicit EncodedInstruction(InstructionType type, unsigned idx) noexcept :
      _idx{idx}, _encoded{Instr{type, nullopt, nullopt}} {}
  explicit EncodedInstruction(string_view sv, unsigned refInstrIdx) noexcept :
      _idx{refInstrIdx}, _encoded{Label{sv}} {}

  auto feed(string_view sv, bool final) -> FeedResult {
    using enum FeedResult;
    if (sv == ":" || sv == ";") {
      return AcceptedFinished;
    }

    assert(holds_alternative<Instr>(_encoded) && "Unexpected parametrized label");
    auto& [type, _0, _1] = get<Instr>(_encoded);
    auto const [minParamCount, maxParamCount] = instructionOpCount(type);
    auto const paramCount = currentParameterCount();
    if (paramCount == maxParamCount) {
      return Full;
    }
    addParam(makeParam(sanitize(sv)));
    if (paramCount == maxParamCount) {
      return AcceptedFinished;
    }

    if (final && currentParameterCount() < minParamCount) {
      throw InvalidTokenException(";", 0, sv.length());
    }

    return final ? AcceptedFinished : Accepted;
  }

  [[nodiscard]] auto index() const noexcept {
    return _idx;
  }

  template <typename IfInstr, typename IfLabel> [[nodiscard]]
  decltype(auto) visit(IfInstr&& ifInstr, IfLabel&& ifLabel) {
    return std::visit([this, &ifLabel, &ifInstr]<typename DT>(DT&& val) {
      using T = remove_cvref_t<DT>;
      if constexpr (is_same_v<T, Instr>) {
        auto&& [type, p0, p1] = val;
        return invoke(ifInstr, type, std::move(p0), std::move(p1));
      } else if constexpr (is_same_v<T, Label>) {
        return invoke(ifLabel, std::move(std::forward<DT>(val)), _idx);
      } else {
        assert(false && "Unhandled EncodedInstruction alternative");
        terminate();
      }
    }, _encoded);
  }

  [[nodiscard]] auto incomplete() const noexcept {
    return holds_alternative<Instr>(_encoded)
        && currentParameterCount() < get<0>(instructionOpCount(get<0>(get<Instr>(_encoded))));
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
          return strtol(sv.data() + 1, &end, 8);
        }
      }
      return strtol(sv.data(), &end, 10);
    }();

    if (end < sv.data() + sv.length()) {
      throw InvalidTokenException(sv);
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

class UndefinedReferenceException : public exception {
public:
  UndefinedReferenceException(string_view referencedIdentifier, EncodedInstruction const& referencedFrom) :
      _identifier{referencedIdentifier}, _pInstruction{&referencedFrom} {}

  [[nodiscard]] auto identifier() const noexcept -> string_view {
    return _identifier;
  }

  [[nodiscard]] auto referencedFrom() const noexcept -> EncodedInstruction const* {
    return _pInstruction;
  }

  [[nodiscard]] auto what() const noexcept -> char const* override {
    return "";
  }

private:
  string _identifier;
  EncodedInstruction const* _pInstruction {nullptr};
};

class Tokenizer {
public:
  auto feed(string_view token) -> optional<EncodedInstruction> {
    using enum FeedResult;
    if (_lineComment || !_current && token == "//") {
      _lineComment = true;
      return nullopt;
    }

    if (token == ",") {
      return nullopt;
    }

    auto finalToken = false;
    if (token.ends_with(';')) {
      finalToken = true;
      token.remove_suffix(1);
    }

    if (!_current) {
      if (auto const opToken = op(sanitize(token))) {
        _current.emplace(*opToken, _instructionIndex++);
        if (finalToken) {
          if (_current->incomplete()) {
            throw InvalidTokenException(";", 0, token.length());
          }
          auto current = std::move(_current);
          _current.reset();
          return current;
        }
      } else {
        return EncodedInstruction{validateLabel(token), _instructionIndex};
      }
    } else {
      auto res = _current->feed(token, finalToken);
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

  [[nodiscard]] auto incomplete() const noexcept -> bool {
    return _current && _current->incomplete();
  }

private:
  bool _lineComment {false};
  unsigned _instructionIndex {0};
  optional<EncodedInstruction> _current {nullopt};
};

class CxxParser {
public:
  explicit CxxParser(string&& code) : _code{std::move(code)} {
    _possibleConstants.resize(numeric_limits<Register>::max());
    iota(_possibleConstants.begin(), _possibleConstants.end(), 0);
    Tokenizer tokenizer;
    string line;
    unsigned lineIndex = 0;
    while (getline(_code, line)) {
      ++lineIndex;
      tokenizer.newLine();
      stringstream lineBuf{std::move(line)};
      revive(line);
      string token;
      while (lineBuf >> token) {
        try {
          if (auto maybeInstruction = tokenizer.feed(token)) {
            _encodedInstructions.push_back(std::move(*maybeInstruction));
          }
        } catch (InvalidTokenException const& tokenException) {
          throw LocatedInvalidTokenException(tokenException, lineIndex, lineBuf.str().find(token) + 1);
        }
      }
    }

    if (auto maybeInstruction = tokenizer.anyRemaining()) {
      _encodedInstructions.push_back(std::move(*maybeInstruction));
    }

    if (tokenizer.incomplete()) {
      throw LocatedInvalidTokenException(InvalidTokenException("<EOF>"), lineIndex + 1, 0);
    }
  }

  auto requiresInvalidation(U16 registerCount, ParserMappedRegister const* pMappedRegisters) noexcept {
    if (_registerMap) {
      auto const& [count, addr, map] = *_registerMap;
      if (count == registerCount && addr == pMappedRegisters) {
        return false;
      }
    }

    _registerMap.emplace();
    auto& [count, addr, map] = *_registerMap;
    count = registerCount;
    addr = pMappedRegisters;
    for (auto end = pMappedRegisters + registerCount; pMappedRegisters != end; ++pMappedRegisters) {
      map.emplace(
          string{pMappedRegisters->pRegisterName, pMappedRegisters->registerNameLength},
          pMappedRegisters->pRegister
      );
    }
    return true;
  }

  auto makeInstructionSet(U16 registerCount, ParserMappedRegister const* pMappedRegisters)
      -> vector<cxx::Instruction> const& {
    if (requiresInvalidation(registerCount, pMappedRegisters)) {
      _cachedInstructions.reset();
    }

    assert(_registerMap && "No valid register map exists");
    if (_cachedInstructions) {
      return *_cachedInstructions;
    }

    _cachedInstructions.emplace();
    _cachedInstructions->reserve(_encodedInstructions.size());
    unordered_map<string_view, unsigned> jumpMap;
    for (auto& encoded : _encodedInstructions) {
      encoded.visit(
          [](auto&&...) {},
          [&jumpMap](auto const& label, unsigned instrRefIdx) {
            jumpMap.emplace(label, instrRefIdx);
          }
      );
    }

    for (auto&& encoded : std::move(_encodedInstructions)) {
      encoded.visit(
          [this, &jumpMap, &encoded](InstructionType type, optional<Parameter>&& p0, optional<Parameter>&& p1) {
            auto paramVisitor = [this, &jumpMap, &encoded](optional<Parameter>&& p) -> Register* {
              if (!p) {
                return nullptr;
              }

              return std::visit([this, &jumpMap, &regMap= get<2>(*_registerMap), &encoded]<typename DT>(DT&& val) -> Register* {
                using T = remove_cvref_t<DT>;
                if constexpr (is_same_v<T, Reference>) {
                  if (auto jumpAt = jumpMap.find(val); jumpAt != jumpMap.end()) {
                    return _possibleConstants.data() + jumpAt->second;
                  } else if (auto reg = regMap.find(val); reg != regMap.end()) {
                    return reg->second;
                  } else {
                    throw UndefinedReferenceException(val, encoded);
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
  optional<vector<cxx::Instruction>> _cachedInstructions;
  vector<Register> _possibleConstants;
  optional<tuple<U16, ParserMappedRegister const*, unordered_map<string, Register*>>> _registerMap {nullopt};
};
} // namespace

extern "C" {
typedef struct Parser_T {
  CxxParser parser;
} Parser_T;

ParserError createParser(ParserCreateInfo const* pCreateInfo, Parser_T** pParser) {
  if (!pCreateInfo || !pParser || !pCreateInfo->pData) {
    return PARSER_ERROR_ILLEGAL_PARAMETER;
  }

  try {
    assert(pCreateInfo->inputType == PARSER_INPUT_TYPE_CODE || pCreateInfo->inputType == PARSER_INPUT_TYPE_FILE_PATH);
    auto const dataLength = pCreateInfo->dataLength == 0u
        ? char_traits<char>::length(pCreateInfo->pData)
        : pCreateInfo->dataLength;
    auto data = pCreateInfo->inputType == PARSER_INPUT_TYPE_FILE_PATH
        ? parsePath({pCreateInfo->pData, dataLength})
        : string{pCreateInfo->pData, dataLength};
    *pParser = new Parser_T{.parser{std::move(data)}};
    return PARSER_ERROR_NONE;
  } catch (LocatedInvalidTokenException const& invalidTokenException) {
    if (auto* pInvalidTokenOutput =
        cxx::find<STRUCTURE_TYPE_PARSER_INVALID_TOKEN_OUTPUT_INFO>(pCreateInfo->pNext)) {
      auto const token = invalidTokenException.token();
      auto const line = invalidTokenException.line();
      auto const column = invalidTokenException.column();

      if (!pInvalidTokenOutput->pToken) {
        return PARSER_ERROR_ILLEGAL_PARAMETER;
      }

      if (pInvalidTokenOutput->tokenLength <= token.length()) { // Includes '\0'
        return PARSER_ERROR_ARRAY_TOO_SMALL;
      }

      pInvalidTokenOutput->line = line;
      pInvalidTokenOutput->column = column;
      pInvalidTokenOutput->tokenLength = token.length();
      char_traits<char>::copy(pInvalidTokenOutput->pToken, token.data(), token.length());
      *(pInvalidTokenOutput->pToken + pInvalidTokenOutput->tokenLength) = '\0';
    }
    return PARSER_ERROR_INVALID_TOKEN;
  } catch (InvalidPathException const&) {
    return PARSER_ERROR_INVALID_PATH;
  } catch (exception const& e) {
    ignore = e;
    return PARSER_ERROR_UNKNOWN;
  }
}

void destroyParser(Parser parser) {
  delete parser;
}

ParserError getParserInstructionSet(
    Parser parser,
    ParserGetInstructionSetInfo const* pGetInfo,
    U16* pInstructionCount,
    Instruction* pInstructions
) {
  if (parser == nullptr || pGetInfo == nullptr || pInstructionCount == nullptr) {
    return PARSER_ERROR_ILLEGAL_PARAMETER;
  }

  try {
    auto const &instructions =
        parser->parser.makeInstructionSet(pGetInfo->mappedRegisterCount, pGetInfo->pMappedRegisters);
    auto givenCount = exchange(*pInstructionCount, instructions.size());
    if (pInstructions) {
      if (givenCount < instructions.size()) {
        return PARSER_ERROR_ARRAY_TOO_SMALL;
      }

      for (auto const &instruction: instructions) {
        *(pInstructions++) = instruction.handle();
      }
    }
    return PARSER_ERROR_NONE;
  } catch (UndefinedReferenceException const& undefinedReferenceException) {
    if (auto* pUndefinedReferenceInfo =
        cxx::find<STRUCTURE_TYPE_PARSER_UNDEFINED_REFERENCE_OUTPUT_INFO>(pGetInfo->pNext)) {
      auto const* pEncoded = undefinedReferenceException.referencedFrom();
      auto const id = undefinedReferenceException.identifier();

      if (!pUndefinedReferenceInfo->pToken) {
        return PARSER_ERROR_ILLEGAL_PARAMETER;
      }

      if (pUndefinedReferenceInfo->tokenLength <= id.length()) { // Includes '\0'
        return PARSER_ERROR_ARRAY_TOO_SMALL;
      }

      pUndefinedReferenceInfo->referencingInstructionIndex = pEncoded->index();
      pUndefinedReferenceInfo->tokenLength = id.length();
      char_traits<char>::copy(pUndefinedReferenceInfo->pToken, id.data(), id.length());
      *(pUndefinedReferenceInfo->pToken + pUndefinedReferenceInfo->tokenLength) = '\0';
    }
    return PARSER_ERROR_UNDEFINED_REFERENCE;
  } catch (exception const& e) {
    return PARSER_ERROR_UNKNOWN;
  }
}
} // extern "C"
