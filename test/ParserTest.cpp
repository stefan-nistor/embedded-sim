//
// Created by loghin on 11/6/24.
//

#include <gtest/gtest.h>
#include <parser/parser.h>

namespace {
using std::string_view;
using std::vector;

template <typename Fn, typename... Args> concept Callable = std::is_invocable_v<Fn, Args...>;

class ParserRAII {
public:
  explicit ParserRAII(string_view code) : _handle{Parser_fromSizedCode(code.length(), code.data())} {}
  ~ParserRAII() noexcept { Parser_destruct(_handle); }
  explicit(false) operator Parser() const noexcept { return _handle;}

private:
  Parser _handle;
};

template <Callable<vector<Instruction> const&> C> auto withInstructionsOf(string_view code, C&& cb) {
  ParserRAII parser(code);
  Instruction const* pInstructions;
  auto count = Parser_getInstructionSet(parser, &pInstructions);
  vector<Instruction> instructions{pInstructions, pInstructions + count};
  return std::invoke(std::forward<C>(cb), instructions);
}
} // namespace

TEST(ParserTest, InstructionCountShouldBeValid) {
  auto count = withInstructionsOf(R"(
mov r0 r1;
mov r1 r2;
// abcd
add r3 r4;
add r3 2;
// haha
// haha2
sub r2 0;
mul r4, 3;
)", [](auto const& instructions) { return instructions.size(); });

  ASSERT_EQ(6, count);
}
