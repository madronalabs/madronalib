// madronalib: a C++ framework for DSP applications.
// Copyright (c) 2020 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLPath.h"
#include "MLTextUtils.h"
#include "utf.hpp"

namespace ml
{
// parse an input string into our representation: an array of ml::Symbols.
Path::Path(const char* str) { parsePathString(str); }

// allocate a path with one symbol.
Path::Path(const ml::Symbol sym) { addSymbol(sym); }

Path::Path(const ml::TextFragment frag) { parsePathString(frag.getText()); }

Path::Path(const ml::TextFragment frag, const char separator)
{
  parsePathString(frag.getText(), separator);
}

Path::Path(const Path& a, const Path& b)
{
  for (Symbol s : a){addSymbol(s);}
  for (Symbol s : b){addSymbol(s);}
}

Path::Path(const Path& a, const Path& b, const Path& c)
{
  for (Symbol s : a){addSymbol(s);}
  for (Symbol s : b){addSymbol(s);}
  for (Symbol s : c){addSymbol(s);}
}

Path::Path(const Path& a, const Path& b, const Path& c, const Path& d)
{
  for (Symbol s : a){addSymbol(s);}
  for (Symbol s : b){addSymbol(s);}
  for (Symbol s : c){addSymbol(s);}
  for (Symbol s : d){addSymbol(s);}
}

inline size_t codepointSize(utf::codepoint_type c)
{
  return utf::internal::utf_traits<utf::utf8>::write_length(c);
}

void Path::parsePathString(const char* pathStr, const char separator)
{
  if (!pathStr) return;

  auto it = TextFragment::Iterator(pathStr);
  const char* symbolStartPtr = pathStr;

  do
  {
    // read one symbol
    size_t codepointSizeInBytes = 0;
    size_t symbolSizeInBytes = 0;

    // skip zero or more separators (which must have codepoint size = 1)
    while(*it == separator)
    {
      symbolStartPtr++;
      ++it;
    }
      
    // advance by code points to end of symbol
    while((*it != separator) && (*it != '\0'))
    {
      // codepointSize(0) is 0, so this is OK
      symbolSizeInBytes += codepointSize(*it);
      ++it;
    }
    
    // create and add the new symbol
    if(symbolSizeInBytes > 0)
    {
      addSymbol(Symbol(symbolStartPtr, symbolSizeInBytes));
      symbolStartPtr += symbolSizeInBytes;
    }
  }
  while (*it != '\0');

  /*
   // TODO benchmark this probably slower implementation out of curiosity
  auto segments = textUtils::split(TextFragment(pathStr), separator);
  for(auto & seg : segments)
  {
    addSymbol(Symbol(seg.getText()));
  }
  */

}

std::ostream& operator<<(std::ostream& out, const ml::Path& r)
{
  out << pathToText(r);
  unsigned copy = r.getCopy();
  if (copy)
  {
    out << "(#" << copy << ")";
  }
  return out;
}

Symbol head(Path p)
{
  if(p.getSize() > 0)
  {
    return p._symbols[0];
  }
  else
  {
    return Symbol();
  }
}

Path tail(Path p)
{
  Path r;
  r.setCopy(p.getCopy());  // to remove
  for (int n = 1; n < p.mSize; ++n)
  {
    r.addSymbol(p._symbols[n]);
  }
  return r;
}

Path butLast(Path p)
{
  Path r;
  for (int n = 0; n < p.mSize - 1; ++n)
  {
    r.addSymbol(p._symbols[n]);
  }
  return r;
}

Symbol last(Path p)
{
  auto len = p.getSize();
  if(len > 0)
  {
    return p._symbols[len - 1];
  }
  return Symbol();
}

Path lastN(Path p, size_t n)
{
  auto len = p.getSize();
  if(len > 1)
  {
    return Path(p._symbols[len - 2], p._symbols[len - 1]);
  }
  return Path();
}

Path substitute(Path p, Symbol from, Symbol to)
{
  Path r{p};
  for (int n = 0; n < p.mSize; ++n)
  {
    if(p._symbols[n] == from)
    {
      r._symbols[n] = to;
    }
  }
  return r;
}

// return a new Path created by substituting every symbol in p matching fromSymbol
// with the Path toPath
Path substitute(Path p, Symbol fromSymbol, Path toPath)
{
  Path r;
  for (int n = 0; n < p.mSize; ++n)
  {
    Symbol next = p._symbols[n];
    if(next == fromSymbol)
    {
      r = Path{r, toPath};
    }
    else
    {
      r = Path{r, Path(next)};
    }
  }
  return r;
}


}  // namespace ml
