//
// Created by loghin on 11/8/24.
//

#pragma once

#include <Types.h>

namespace cxx {
template <StructureType type> struct FindStructureTypeResultImpl {};

template <> struct FindStructureTypeResultImpl<STRUCTURE_TYPE_PARSER_INVALID_TOKEN_OUTPUT_INFO> {
  using Type = ParserInvalidTokenOutputInfo;
};

template <> struct FindStructureTypeResultImpl<STRUCTURE_TYPE_PARSER_UNDEFINED_REFERENCE_OUTPUT_INFO> {
  using Type = ParserUndefinedReferenceOutputInfo;
};

template <StructureType type> using FindStructureTypeResult = typename FindStructureTypeResultImpl<type>::Type;

template <StructureType type> auto find(void* pChain) noexcept -> FindStructureTypeResult<type>* {
  if (pChain == nullptr) {
    return nullptr;
  }

  auto pGeneric = static_cast<OutStructure*>(pChain);
  return pGeneric->structureType == type
         ? static_cast<FindStructureTypeResult<type>*>(pChain)
         : find<type>(pGeneric->pNext);
}

template <StructureType type> auto find(void const* pChain) noexcept -> FindStructureTypeResult<type>* {
  if (pChain == nullptr) {
    return nullptr;
  }

  auto pGeneric = static_cast<InStructure const*>(pChain);
  return pGeneric->structureType == type
         ? static_cast<FindStructureTypeResult<type> const*>(pChain)
         : find<type>(pGeneric->pNext);
}
} // namespace cxx
