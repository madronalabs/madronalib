
// ml::Symbol.cpp
// ----------
// Madrona Labs C++ framework for DSP applications.
// Copyright (c) 2015 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLSymbol.h"

namespace ml
{
#pragma mark SymbolTable

SymbolTable::SymbolTable() { clear(); }

SymbolTable::~SymbolTable()
{
  // TODO better protection on delete
  // can this be avoided with more explicit setup / shutdown
  // (RAII in main() )
  // std::unique_lock<std::mutex> lock(mMutex);
}

// clear all symbols from the table.
void SymbolTable::clear()
{
  mSize = 0;
  // std::unique_lock<std::mutex> lock(mMutex);

  mSymbolTextsByID.clear();
  mSymbolTextsByID.reserve(kDefaultSymbolTableSize);

  for (int i = 0; i < kHashTableSize; ++i)
  {
    clearEntry(mHashTable[i]);
  }

  // add null entry - why?
  addEntry(HashedCharArray());
}

// add an entry to the table. The entry must not already exist in the table.
// this must be the only way of modifying the symbol table.
SymbolID SymbolTable::addEntry(const HashedCharArray& hsl)
{
  mSymbolTextsByID.emplace_back(TextFragment(hsl.pChars, static_cast<int>(hsl.len)));

  size_t newID = mSize++;
  mHashTable[hsl.hash].mIDVector.emplace_back(newID);
  return newID;
}

SymbolID SymbolTable::getSymbolID(const HashedCharArray& hsl)
{
  SymbolID r = 0;

  // get the vector of symbol IDs matching this hash. It probably has one entry
  // but may have more.
  const std::vector<SymbolID>& bin = mHashTable[hsl.hash].mIDVector;
  {
    bool found = false;

    std::unique_lock<std::mutex> lock(mHashTable[hsl.hash].mMutex);

    for (auto ID : bin)
    {
      // there should be few collisions, so probably the first ID in the hash
      // bin will be the symbol we are looking for. Unfortunately to test for
      // equality we may have to compare the entire string.
      TextFragment* binFragment = &mSymbolTextsByID[ID];
      if (compareSizedCharArrays(binFragment->getText(), binFragment->lengthInBytes(), hsl.pChars,
                                 hsl.len))
      {
        r = ID;
        found = true;
        break;
      }
    }

    if (!found)
    {
      mSymbolTextsByID.emplace_back(TextFragment(hsl.pChars, static_cast<int>(hsl.len)));
      r = mSize++;
      mHashTable[hsl.hash].mIDVector.emplace_back(r);
    }
  }
  return r;
}

SymbolID SymbolTable::getSymbolID(const char* sym) { return getSymbolID(HashedCharArray(sym)); }

SymbolID SymbolTable::getSymbolID(const char* sym, size_t lengthBytes)
{
  return getSymbolID(HashedCharArray(sym, lengthBytes));
}

const TextFragment& SymbolTable::getSymbolTextByID(SymbolID symID)
{
  return mSymbolTextsByID[symID];
}

void SymbolTable::dump()
{
  std::cout << "---------------------------------------------------------\n";
  std::cout << mSymbolTextsByID.size() << " symbols:\n";

  // print symbols in order of creation.
  for (int i = 0; i < mSymbolTextsByID.size(); ++i)
  {
    const TextFragment& sym = mSymbolTextsByID[i];
    std::cout << "    ID " << i << " = " << sym << "\n";
  }
  // print nonzero entries in hash table
  int hash = 0;
  for (auto& tableEntry : mHashTable)
  {
    auto idVec = tableEntry.mIDVector;
    size_t idVecLen = idVec.size();
    if (idVecLen > 0)
    {
      std::cout << "#" << hash << " ";
      for (auto id : idVec)
      {
        std::cout << id << " " << getSymbolTextByID(id) << " ";
      }

      std::cout << "\n";
    }
    hash++;
  }
}

int SymbolTable::audit()
{
  int i = 0;
  SymbolID i2{0};
  bool OK = true;
  size_t size = mSymbolTextsByID.size();

  for (i = 0; i < size; ++i)
  {
    const TextFragment& sym = getSymbolTextByID(i);
    const char* symChars = sym.getText();
    Symbol symB(symChars);

    i2 = symB.getID();
    if (i != i2)
    {
      OK = false;
      break;
    }
    if (i2 > size)
    {
      OK = false;
      break;
    }
  }
  if (!OK)
  {
    const TextFragment& s = getSymbolTextByID(i);
    std::cout << "SymbolTable: error in symbol table, line " << i << ":\n";
    std::cout << "    ID " << i << " = " << s << ", ID B = " << i2 << "\n";
  }
  return OK;
}

std::ostream& operator<<(std::ostream& out, const Symbol r)
{
  out << r.getTextFragment();
  return out;
}

}  // namespace ml
