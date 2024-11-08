//
// Created by loghin on 11/6/24.
//

#include <gtest/gtest.h>
#include <parser/parser.h>

#include "InstructionMock.hpp"

namespace {
using std::string_view;
using std::vector;

using namespace testing::mock;

template <typename Fn, typename... Args> concept Callable = std::is_invocable_v<Fn, Args...>;

class ParserException : public std::exception {
public:
  explicit ParserException(ParserError error) : _error{error} {}
  ParserException(std::string token, unsigned line, unsigned column) :
      _token{std::move(token)}, _line{line}, _column{column} {}

  [[nodiscard]] auto error() const {
    return _error;
  }

  [[nodiscard]] auto const& token() const {
    return _token;
  }

  [[nodiscard]] auto line() const {
    return _line;
  }

  [[nodiscard]] auto column() const {
    return _column;
  }

private:
  ParserError _error {PARSER_ERROR_NONE};
  std::string _token;
  unsigned _line {0};
  unsigned _column {0};
};

class ParserRAII {
public:
  explicit ParserRAII(string_view code) : _handle{create(code)} {}
  ~ParserRAII() noexcept { destroyParser(_handle); }

  [[nodiscard]] auto instructions(vector<ParserMappedRegister> const& mappedRegisters) const -> vector<Instruction> {
    U16 instructionCount;
    ParserGetInstructionSetInfo getInfo {
      .structureType = STRUCTURE_TYPE_PARSER_GET_INSTRUCTION_SET_INFO,
      .pNext = nullptr,
      .mappedRegisterCount = static_cast<U16>(mappedRegisters.size()),
      .pMappedRegisters = mappedRegisters.data()
    };
    if (auto const error = getParserInstructionSet(_handle, &getInfo, &instructionCount, nullptr);
        error != PARSER_ERROR_NONE) {
      throw ParserException(error);
    }

    vector<Instruction> instructions;
    instructions.resize(instructionCount, nullptr);
    if (auto const error = getParserInstructionSet(_handle, &getInfo, &instructionCount, instructions.data());
        error != PARSER_ERROR_NONE) {
      throw ParserException(error);
    }

    return instructions;
  }

private:
  static auto create(string_view code) -> Parser {
    std::string invalidTokenBuffer(128, '\0');
    auto invalidTokenInfo = ParserInvalidTokenOutputInfo {
      .structureType = STRUCTURE_TYPE_PARSER_INVALID_TOKEN_OUTPUT_INFO,
      .pNext = nullptr,
      .line = 0,
      .column = 0,
      .tokenLength = 128,
      .pToken = invalidTokenBuffer.data(),
    };

    auto createInfo = ParserCreateInfo {
      .structureType = STRUCTURE_TYPE_PARSER_CREATE_INFO,
      .pNext = &invalidTokenInfo,
      .inputType = PARSER_INPUT_TYPE_CODE,
      .dataLength = static_cast<U32>(code.length()),
      .pData = code.data()
    };

    Parser parser = nullptr;
    if (auto const error = createParser(&createInfo, &parser);
        error == PARSER_ERROR_INVALID_TOKEN) {
      invalidTokenBuffer.resize(invalidTokenInfo.tokenLength);
      throw ParserException(std::move(invalidTokenBuffer), invalidTokenInfo.line, invalidTokenInfo.column);
    } else if (error != PARSER_ERROR_NONE) {
      throw ParserException(error);
    }

    return parser;
  }

  Parser _handle;
};
} // namespace

TEST(ParserTest, InstructionCountShouldBeValid) {
  auto code = R"(
mov r0 r1;
mov r1 r2;
// abcd
add r3 r4;
add r3 2;
// hah
// hah2
sub r2 0;
mul r4, 3;
)";

  ParserRAII parser{code};

  ASSERT_EQ(6, parser.instructions({}).size());
}

TEST(ParserTest, InstructionTypesShouldBeValid) {
  auto code = R"(
mov r0 r1;
mov r1 r2;
// abcd
add r3 r4;
add r3 2;
// hah
// hah2
sub r2 0;
mul r4, 3;
)";

  ParserRAII parser{code};

  ASSERT_EQ(instructions(
      mov(), mov(), add(), add(), sub(), mul()
  ), parser.instructions({}));
}

TEST(ParserTest, InstructionConstantsShouldBeValid) {
  auto code = R"(
mov r0 r1;
mov r1 54;
// abcd
add r3 4;
add r3 2;
// hah
// hah2
sub r2 0;
mul r4, 3;
)";

  ParserRAII parser{code};

  ASSERT_EQ(instructions(
      mov(), mov(any(), 54), add(any(), 4), add(any(), 2), sub(any(), 0), mul(any(), 3)
  ), parser.instructions({}));
}

TEST(ParserTest, InstructionConstantInBase2ShouldBeValid) {
  auto code = R"(
mov r0 0b1011;
)";

  ParserRAII parser{code};

  ASSERT_EQ(instructions(
      mov(any(), 0b1011)
  ), parser.instructions({}));
}

TEST(ParserTest, InstructionConstantInBase8ShouldBeValid) {
  auto code = R"(
mov r0 0766;
)";

  ParserRAII parser{code};

  ASSERT_EQ(instructions(
      mov(any(), 0766)
  ), parser.instructions({}));
}

TEST(ParserTest, InstructionConstantInBase16ShouldBeValid) {
  auto code = R"(
mov r0 0xDEAD;
)";

  ParserRAII parser{code};

  ASSERT_EQ(instructions(
      mov(any(), 0xDEAD)
  ), parser.instructions({}));
}

TEST(ParserTest, InstructionConstantInvalidShouldYieldError) {
  auto code = R"(
mov r0 10abc;
)";

  try {
    ParserRAII parser{code};
  } catch (ParserException const& exception) {
    ASSERT_EQ("10abc", exception.token());
    ASSERT_EQ(2, exception.line());
    ASSERT_EQ(8, exception.column());
  }
}

TEST(ParserTest, CommasDoNothing) {
  auto code = R"(
mov r0 , 10;
)";

  ParserRAII parser{code};
  ASSERT_EQ(instructions(mov(any(), 10)), parser.instructions({}));
}

TEST(ParserTest, CommasDoNothing2) {
  auto code = R"(
mov r0 , 10;
)";

  ParserRAII parser{code};
  ASSERT_EQ(instructions(mov(any(), 10)), parser.instructions({}));
}

TEST(ParserTest, CreateParserWithInvalidArgsYieldsError) {
  ParserCreateInfo createInfo {
    .structureType = STRUCTURE_TYPE_PARSER_CREATE_INFO,
    .pNext = nullptr,
    .inputType = PARSER_INPUT_TYPE_FILE_PATH,
    .dataLength = 0,
    .pData = nullptr
  };

  Parser p;

  ASSERT_EQ(PARSER_ERROR_ILLEGAL_PARAMETER, createParser(nullptr, &p));
  ASSERT_EQ(PARSER_ERROR_ILLEGAL_PARAMETER, createParser(&createInfo, nullptr));
  ASSERT_EQ(PARSER_ERROR_ILLEGAL_PARAMETER, createParser(&createInfo, &p));
}

TEST(ParserTest, CreateParserWithInvalidPathYieldsError) {
  ParserCreateInfo createInfo {
    .structureType = STRUCTURE_TYPE_PARSER_CREATE_INFO,
    .pNext = nullptr,
    .inputType = PARSER_INPUT_TYPE_FILE_PATH,
    .dataLength = 0,
    .pData = "/this/is/an/invalid/path" // Now I hope no OS supports this path.
  };

  Parser p;
  ASSERT_EQ(PARSER_ERROR_INVALID_PATH, createParser(&createInfo, &p));
}

TEST(ParserTest, CreateParserWithInvalidCodeYieldsTokenError) {
  ParserCreateInfo createInfo {
    .structureType = STRUCTURE_TYPE_PARSER_CREATE_INFO,
    .pNext = nullptr,
    .inputType = PARSER_INPUT_TYPE_CODE,
    .dataLength = 0,
    .pData = R"(
0xDEAD
)"
  };

  Parser p;
  ASSERT_EQ(PARSER_ERROR_INVALID_TOKEN, createParser(&createInfo, &p));
}

TEST(ParserTest, CreateParserWithInvalidCodeAndInvalidTokenOutputStructureYieldsError) {
  ParserInvalidTokenOutputInfo invalidTokenInfo {
    .structureType = STRUCTURE_TYPE_PARSER_INVALID_TOKEN_OUTPUT_INFO,
    .pNext = nullptr,
    .line = 0,
    .column = 0,
    .tokenLength = 512,
    .pToken = nullptr
  };
  ParserCreateInfo createInfo {
    .structureType = STRUCTURE_TYPE_PARSER_CREATE_INFO,
    .pNext = &invalidTokenInfo,
    .inputType = PARSER_INPUT_TYPE_CODE,
    .dataLength = 0,
    .pData = R"(
0xDEAD
)"
  };

  Parser p;
  ASSERT_EQ(PARSER_ERROR_ILLEGAL_PARAMETER, createParser(&createInfo, &p));
}

TEST(ParserTest, CreateParserWithInvalidCodeAndTokenOutputTooSmallYieldsError) {
  char buffer[2];
  ParserInvalidTokenOutputInfo invalidTokenInfo {
    .structureType = STRUCTURE_TYPE_PARSER_INVALID_TOKEN_OUTPUT_INFO,
    .pNext = nullptr,
    .line = 0,
    .column = 0,
    .tokenLength = 2, // shorter than "0xDEAD"
    .pToken = buffer
  };
  ParserCreateInfo createInfo {
    .structureType = STRUCTURE_TYPE_PARSER_CREATE_INFO,
    .pNext = &invalidTokenInfo,
    .inputType = PARSER_INPUT_TYPE_CODE,
    .dataLength = 0,
    .pData = R"(
0xDEAD
)"
  };

  Parser p;
  ASSERT_EQ(PARSER_ERROR_ARRAY_TOO_SMALL, createParser(&createInfo, &p));
}

TEST(ParserTest, CreateParserWithInvalidCodeAndTokenOutpuLocatesInvalidToken) {
  std::string buffer;
  buffer.reserve(512);
  ParserInvalidTokenOutputInfo invalidTokenInfo {
    .structureType = STRUCTURE_TYPE_PARSER_INVALID_TOKEN_OUTPUT_INFO,
    .pNext = nullptr,
    .line = 0,
    .column = 0,
    .tokenLength = 512,
    .pToken = buffer.data()
  };
  ParserCreateInfo createInfo {
    .structureType = STRUCTURE_TYPE_PARSER_CREATE_INFO,
    .pNext = &invalidTokenInfo,
    .inputType = PARSER_INPUT_TYPE_CODE,
    .dataLength = 0,
    .pData = R"(mov r0 r1;
mov r1 r2;
    0xDEAD;
add r1 r3;)"
  };

  Parser p;
  ASSERT_EQ(PARSER_ERROR_INVALID_TOKEN, createParser(&createInfo, &p));
  ASSERT_EQ(3, invalidTokenInfo.line);
  ASSERT_EQ(5, invalidTokenInfo.column);
  ASSERT_EQ(6, invalidTokenInfo.tokenLength);
  ASSERT_EQ(buffer.data(), invalidTokenInfo.pToken);
  ASSERT_EQ("0xDEAD", string_view(buffer.data(), 6));
}

TEST(ParserTest, GetInstructionSetWithInvalidArgsYieldsError) {
  Parser p;
  ParserCreateInfo createInfo {
    .structureType = STRUCTURE_TYPE_PARSER_CREATE_INFO,
    .pNext = nullptr,
    .inputType = PARSER_INPUT_TYPE_CODE,
    .dataLength = 0,
    .pData = R"(mov r0 r1; mov r1 r2; mov r2 r0;)"
  };
  ASSERT_EQ(PARSER_ERROR_NONE, createParser(&createInfo, &p));
  ParserGetInstructionSetInfo getInfo {
    .structureType = STRUCTURE_TYPE_PARSER_GET_INSTRUCTION_SET_INFO,
    .pNext = nullptr,
    .mappedRegisterCount = 0u,
    .pMappedRegisters = nullptr
  };
  U16 iCount;
  Instruction ins[2];
  ASSERT_EQ(PARSER_ERROR_ILLEGAL_PARAMETER, getParserInstructionSet(nullptr, &getInfo, &iCount, ins));
  ASSERT_EQ(PARSER_ERROR_ILLEGAL_PARAMETER, getParserInstructionSet(p, nullptr, &iCount, ins));
  ASSERT_EQ(PARSER_ERROR_ILLEGAL_PARAMETER, getParserInstructionSet(p, &getInfo, nullptr, ins));
}

TEST(ParserTest, GetInstructionSetWithSmallInstructionArrayYieldsError) {
  Parser p;
  ParserCreateInfo createInfo {
    .structureType = STRUCTURE_TYPE_PARSER_CREATE_INFO,
    .pNext = nullptr,
    .inputType = PARSER_INPUT_TYPE_CODE,
    .dataLength = 0,
    .pData = R"(mov r0 r1; mov r1 r2; mov r2 r0;)"
  };
  ASSERT_EQ(PARSER_ERROR_NONE, createParser(&createInfo, &p));
  ParserGetInstructionSetInfo getInfo {
    .structureType = STRUCTURE_TYPE_PARSER_GET_INSTRUCTION_SET_INFO,
    .pNext = nullptr,
    .mappedRegisterCount = 0u,
    .pMappedRegisters = nullptr
  };
  U16 iCount = 2;
  Instruction ins[2]; // < 3
  ASSERT_EQ(PARSER_ERROR_ARRAY_TOO_SMALL, getParserInstructionSet(p, &getInfo, &iCount, ins));
  ASSERT_EQ(3, iCount);
}

TEST(ParserTest, GetInstructionSetWithNoBufferYieldsRequiredSize) {
  Parser p;
  ParserCreateInfo createInfo {
    .structureType = STRUCTURE_TYPE_PARSER_CREATE_INFO,
    .pNext = nullptr,
    .inputType = PARSER_INPUT_TYPE_CODE,
    .dataLength = 0,
    .pData = R"(mov r0 r1; mov r1 r2; mov r2 r0;)"
  };
  ASSERT_EQ(PARSER_ERROR_NONE, createParser(&createInfo, &p));
  ParserGetInstructionSetInfo getInfo {
    .structureType = STRUCTURE_TYPE_PARSER_GET_INSTRUCTION_SET_INFO,
    .pNext = nullptr,
    .mappedRegisterCount = 0u,
    .pMappedRegisters = nullptr
  };
  U16 iCount = 2;
  ASSERT_EQ(PARSER_ERROR_NONE, getParserInstructionSet(p, &getInfo, &iCount, nullptr));
  ASSERT_EQ(3, iCount);
}

TEST(ParserTest, JumpParseIsRecognizedAndLabelIsIgnored) {
  auto code = R"(
mov r0 10;
test:
mov r1 20;
jmp test;
mov r2 30;
)";

  ParserRAII parser{code};

  ASSERT_EQ(instructions(
      mov(),
      mov(),
      jmp(),
      mov()
  ), parser.instructions({}));
}

TEST(ParserTest, JumpParseYieldsCorrectInstructionIndex) {
  auto code = R"(
mov r0 10;
test:
mov r1 20;
jmp test;
mov r2 30;
)";

  ParserRAII parser{code};

  ASSERT_EQ(instructions(
      mov(),
      mov(),
      jmp(1),
      mov()
  ), parser.instructions({}));
}

TEST(ParserTest, JumpParseHasSecondArgNull) {
  auto code = R"(
mov r0 10;
test:
mov r1 20;
jmp test;
mov r2 30;
)";

  ParserRAII parser{code};

  ASSERT_EQ(instructions(
      mov(),
      mov(),
      jmp(any(), nullptr),
      mov()
  ), parser.instructions({}));
}

TEST(ParserTest, InstructionWithTooManyParametersYieldsError) {
  auto code = R"(
mov r0 10 20;
test:
mov r1 20;
)";

  try {
    ParserRAII parser{code};
    ASSERT_FALSE(true);
  } catch (ParserException const& e) {
    ASSERT_EQ(2, e.line());
    ASSERT_EQ(11, e.column());
    ASSERT_EQ("20", e.token());
  }
}

TEST(ParserTest, InstructionWithTooFewParametersYieldsError) {
  auto code = R"(
mov r0;
)";

  try {
    ParserRAII parser{code};
    ASSERT_FALSE(true);
  } catch (ParserException const& e) {
    ASSERT_EQ(";", e.token());
    ASSERT_EQ(2, e.line());
    ASSERT_EQ(7, e.column());
  }
}

TEST(ParserTest, InstructionWithTooFewAndUnterminatedYieldsError) {
  auto code = R"(
mov r0
)";

  try {
    ParserRAII parser{code};
    ASSERT_FALSE(true);
  } catch (ParserException const& e) {
    ASSERT_EQ("<EOF>", e.token());
    ASSERT_EQ(3, e.line());
    ASSERT_EQ(0, e.column());
  }
}