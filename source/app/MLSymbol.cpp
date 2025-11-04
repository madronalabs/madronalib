// madronalib: a C++ framework for DSP applications.
// Copyright (c) 2020-2022 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLSymbol.h"

namespace ml
{

const TextFragment SymbolTable::kNullText{"?"};

uint64_t SymbolTable::registerSymbol(const char* text, size_t len)
{
  uint64_t hash = fnv1aRuntime(text, len);
  
  std::lock_guard<std::mutex> lock(mMutex);
  
  auto it = mSymbols.find(hash);
  if (it != mSymbols.end())
  {
    // Hash exists - check for collision
    const TextFragment& existing = it->second;
    if (existing.lengthInBytes() != len ||
        !compareSizedCharArrays(existing.getText(), existing.lengthInBytes(), text, len))
    {
      // COLLISION DETECTED!
      throw std::runtime_error("Symbol hash collision detected!");
    }
    // Same string, return existing hash
    return hash;
  }
  
  // New symbol - register it
  mSymbols.emplace(hash, TextFragment(text, static_cast<int>(len)));
  return hash;
}

void SymbolTable::clear()
{
  mSymbols.clear();
}

const TextFragment& SymbolTable::getTextForHash(uint64_t hash) const
{
  auto it = mSymbols.find(hash);
  
  // if not found, return null object
  if (it == mSymbols.end()) return SymbolTable::kNullText;
  
  return it->second;
}

void SymbolTable::dump()
{
  std::cout << mSymbols.size() << " symbols:\n";
  
  for (const auto& [hash, text] : mSymbols)
  {
    std::cout << "0x" << std::hex << hash << std::dec << " = \"" << text << "\"\n";
  }
}

std::ostream& operator<< (std::ostream& out, const Symbol r)
{
  out << r.getTextFragment();
  return out;
}


}  // namespace ml
