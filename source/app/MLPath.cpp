// madronalib: a C++ framework for DSP applications.
// Copyright (c) 2020-2022 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLPath.h"

#include "MLTextUtils.h"
#include "utf.hpp"

namespace ml
{

void parsePathStringIntoSymbols(GenericPath<Symbol>& path, const char* pathStr)
{
  constexpr char delimiter{'/'};
  if (!pathStr) return;
  
  auto it = TextFragment::Iterator(pathStr);
  const char* symbolStartPtr = pathStr;
  
  do
  {
    size_t symbolSizeInBytes = 0;
    
    // Skip separators
    while (*it == delimiter)
    {
      symbolStartPtr++;
      ++it;
    }
    
    // Advance to end of symbol
    while ((*it != delimiter) && (*it != '\0'))
    {
      symbolSizeInBytes += utf::internal::utf_traits<utf::utf8>::write_length(*it);
      ++it;
    }
    
    // Create and add the symbol
    if (symbolSizeInBytes > 0)
    {
      path.addElement(runtimeSymbol(symbolStartPtr, symbolSizeInBytes));
      symbolStartPtr += symbolSizeInBytes;
    }
  } while (*it != '\0');
}

void parsePathStringIntoTextFragments(GenericPath<TextFragment>& path, const char* pathStr, const char delimiter)
{
  if (!pathStr) return;
  
  auto it = TextFragment::Iterator(pathStr);
  const char* symbolStartPtr = pathStr;
  
  do
  {
    size_t symbolSizeInBytes = 0;
    
    // Skip separators
    while (*it == delimiter)
    {
      symbolStartPtr++;
      ++it;
    }
    
    // Advance to end of symbol
    while ((*it != delimiter) && (*it != '\0'))
    {
      symbolSizeInBytes += utf::internal::utf_traits<utf::utf8>::write_length(*it);
      ++it;
    }
    
    // Create and add the TextFragment
    if (symbolSizeInBytes > 0)
    {
      path.addElement(TextFragment(symbolStartPtr, static_cast<int>(symbolSizeInBytes)));
      symbolStartPtr += symbolSizeInBytes;
    }
  } while (*it != '\0');
}


}  // namespace ml
