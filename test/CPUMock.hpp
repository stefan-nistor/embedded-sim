//
// Created by loghin on 11/8/24.
//

#pragma once

#include <array>
#include <string>
#include <string_view>
#include <vector>

#include <parser/parser.h>

namespace testing::mock::detail {
using std::array;
using std::string;
using std::string_view;
using std::to_string;
using std::vector;
template <unsigned regCount = 8> class MockCpuRegisterMap {
public:
  explicit MockCpuRegisterMap(string_view prefix = "r", unsigned startingOffset = 0) {
    for (auto& name : _names) {
      name = string{prefix} + to_string(startingOffset++);
    }
    for (auto& reg : _registers) {
      reg = 0;
    }
  }

  [[nodiscard]] auto map() noexcept {
    vector<ParserMappedRegister> mapped;
    mapped.reserve(regCount);
    for (auto idx = 0; idx < regCount; ++idx) {
      mapped.emplace_back(ParserMappedRegister {
          .registerNameLength = static_cast<U32>(_names[idx].length()),
          .pRegisterName = _names[idx].c_str(),
          .pRegister = &_registers[idx]
      });
    }
    return mapped;
  }

  [[nodiscard]] auto const& regs() const noexcept {
    return _registers;
  }

private:
  array<string, regCount> _names;
  array<U16, regCount> _registers;
};
} // namespace testing::mock::detail

namespace testing::mock {
using detail::MockCpuRegisterMap;
} // namespace testing::mock
