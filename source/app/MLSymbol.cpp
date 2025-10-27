// madronalib: a C++ framework for DSP applications.
// Copyright (c) 2020-2022 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLSymbol.h"

namespace ml
{

const TextFragment& SymbolTable::getTextForHash(uint64_t hash) const
{
  auto it = mSymbols.find(hash);
  
  // if not found, return null object
  if (it == mSymbols.end()) return TextFragment();
  
  return it->second;
}


std::ostream& operator<< (std::ostream& out, const Symbol r)
{
  out << r.getTextFragment();
  return out;
}


}  // namespace ml
