// madronalib: a C++ framework for DSP applications.
// Copyright (c) 2020-2022 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLSymbol.h"

#include <mutex>

namespace ml
{

const TextFragment SymbolTable::kNullText{"?"};

uint64_t SymbolTable::registerSymbol(const char* text, size_t len)
{
  uint64_t hash = fnv1aRuntime(text, len);

  std::lock_guard<std::mutex> lock(mutex_);

  // TEMP
  /*
  std::cout << "making ";
  for (int i = 0; i < len; ++i)
  {
    std::cout << text[i];
  }
  std::cout << "\n";
*/


  auto it = symbols_.find(hash);
  if (it != symbols_.end())
  {
    // Hash exists - check for collision
    const TextFragment& existing = it->second;
    if (existing.lengthInBytes() != len ||
        !compareSizedCharArrays(existing.getText(), existing.lengthInBytes(), text, len))
    {
      throw std::runtime_error("Symbol hash collision detected!");
    }
    // Same string, return existing hash
    return hash;
  }

  // New symbol - register it
  symbols_.emplace(hash, TextFragment(text, static_cast<int>(len)));
  return hash;
}

void SymbolTable::clear() { symbols_.clear(); }

const TextFragment& SymbolTable::getTextForHash(uint64_t hash) const
{
  auto it = symbols_.find(hash);

  // if not found, return null object
  if (it == symbols_.end()) return SymbolTable::kNullText;

  return it->second;
}

void SymbolTable::dump()
{
  std::cout << symbols_.size() << " symbols:\n";

  for (const auto& [hash, text] : symbols_)
  {
    std::cout << "0x" << std::hex << hash << std::dec << " = \"" << text << "\"\n";
  }
}

std::ostream& operator<<(std::ostream& out, const Symbol r)
{
  out << r.getTextFragment();
  return out;
}

}  // namespace ml
