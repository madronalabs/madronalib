
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

// converters to/from binary and text formats for various objects.

#pragma once

#include <iostream>
#include <cmath>
#include <list>
#include <map>
#include <cstdlib>
#include <numeric>

#include "MLMatrix.h"
#include "MLSymbol.h"
#include "MLText.h"
#include "MLValue.h"
#include "MLTextUtils.h"

namespace ml{

  struct BinaryGroupHeader
  {
    uint32_t elements;
  };

  struct BinaryChunkHeader
  {
    unsigned int type : 8;
    unsigned int dataBytes : 24;
  };

  struct BinaryMatrixHeader
  {
    unsigned int type : 8;
    unsigned int dataBytes : 24;
    uint32_t width;
    uint32_t height;
    uint32_t depth;
  };


  // Text

  inline TextFragment valueToText(const Value v)
  {
    // TODO


    TextFragment t;
    switch(v.getType())
    {
      default:
      case Value::kUndefinedValue:
        return TextFragment{"U"};
        break;
      case Value::kFloatValue:
        return TextFragment{"F", textUtils::floatNumberToText(v.getFloatValue())};
        break;
      case Value::kTextValue:
        return TextFragment{"T", v.getTextValue()};
        break;
      case Value::kMatrixValue:
        return TextFragment{};// {"M", matrixToText(v.getMatrixValue())};
        break;
    }
    return t;
  }

  inline Value textToValue(const Text v)
  {
    return Value(); // TODO
  }

  inline std::unique_ptr< std::vector<uint8_t> > valueToBinary(Value v)
  {
    std::vector<unsigned char> outputVector;
    auto headerSize = sizeof(BinaryChunkHeader);
    auto matrixHeaderSize = sizeof(BinaryMatrixHeader);

    switch(v.getType())
    {
      default:
      case Value::kUndefinedValue:
      {
        outputVector.resize(sizeof(BinaryChunkHeader));
        BinaryChunkHeader* header {reinterpret_cast<BinaryChunkHeader*>(outputVector.data())};
        *header = BinaryChunkHeader{'U', 0};
        break;
      }
      case Value::kFloatValue:
      {
        float f = v.getFloatValue();
        unsigned dataSize = sizeof(float);
        outputVector.resize(headerSize + dataSize);
        BinaryChunkHeader* header {reinterpret_cast<BinaryChunkHeader*>(outputVector.data())};
        *header = BinaryChunkHeader{'F', dataSize};
        float* pDest{reinterpret_cast< float* >(outputVector.data() + headerSize)};
        *pDest = f;
        break;
      }
      case Value::kTextValue:
      {
        auto textVal = v.getTextValue();
        unsigned dataSize = textVal.lengthInBytes();

        // TODO safety, limits

        outputVector.resize(headerSize + dataSize);
        BinaryChunkHeader* header {reinterpret_cast<BinaryChunkHeader*>(outputVector.data())};
        *header = BinaryChunkHeader{'T', dataSize};
        auto pDest{outputVector.data() + headerSize};
        auto pSrc{textVal.getText()};
        std::copy(pSrc, pSrc + dataSize, pDest);
        break;
      }
      case Value::kMatrixValue:
      {
        auto matrixVal = v.getMatrixValue();

        uint32_t width = matrixVal.getWidth();
        uint32_t height = matrixVal.getHeight();
        uint32_t depth = matrixVal.getDepth();

        unsigned dataSize = width*height*depth*sizeof(float);

        // TODO safety, limits

        outputVector.resize(matrixHeaderSize + dataSize);
        BinaryMatrixHeader* header {reinterpret_cast<BinaryMatrixHeader*>(outputVector.data())};
        *header = BinaryMatrixHeader{'M', dataSize, width, height, depth};
        float* pDest{reinterpret_cast< float* >(outputVector.data() + matrixHeaderSize)};

        matrixVal.writeToPackedData(pDest);
        break;
      }
    }
    return ml::make_unique< std::vector<unsigned char> >(outputVector);
  }

  inline Value binaryToValue(const unsigned char* p)
  {
    Value returnValue{};
    auto header {reinterpret_cast<const BinaryChunkHeader*>(p)};
    switch(header->type)
    {
      default:
      case 'U':
      {
        break;
      }
      case 'F':
      {
        const unsigned char* pData = p + sizeof(BinaryChunkHeader);
        auto* pFloatData{reinterpret_cast<const float*>(pData)};
        float f = *pFloatData;
        returnValue = Value{f};
        break;
      }
      case 'T':
      {
        const unsigned char* pData = p + sizeof(BinaryChunkHeader);
        auto* p{reinterpret_cast<const char*>(pData)};
        returnValue = Value{Text(p, header->dataBytes)};
        break;
      }
      case 'M':
      {
        BinaryMatrixHeader header {*reinterpret_cast<const BinaryMatrixHeader*>(p)};
        const unsigned char* pData = p + sizeof(BinaryMatrixHeader);
        auto* pFloatData{reinterpret_cast<const float*>(pData)};
        Matrix m(header.width, header.height, header.depth);
        m.readFromPackedData(pFloatData);
        returnValue = Value{m};

        break;
      }
    }
    return returnValue;
  }


  // Path

  inline TextFragment pathToText(Path p, const char separator)
  {
    auto concat = [&](Symbol a, Symbol b) { return TextFragment(a.getTextFragment(), TextFragment(separator), b.getTextFragment()); } ;
    return std::accumulate(++p.begin(), p.end(), (*p.begin()).getTextFragment(), concat);
  }

  inline std::unique_ptr< std::vector<unsigned char> > pathToBinary(Path p)
  {
    std::vector<unsigned char> outputVector;
    auto t = pathToText(p, '/');
    auto headerSize = sizeof(BinaryChunkHeader);
    auto dataSize = t.lengthInBytes();
    outputVector.resize(headerSize + dataSize);
    BinaryChunkHeader* header {reinterpret_cast<BinaryChunkHeader*>(outputVector.data())};
    header->type = 'P';
    header->dataBytes = dataSize;

    auto pDest{outputVector.data() + headerSize};
    auto pSrc{t.getText()};
    std::copy(pSrc, pSrc + dataSize, pDest);

    return ml::make_unique< std::vector<unsigned char> >(outputVector);
  }

  inline Path binaryToPath(const uint8_t* p)
  {
    BinaryChunkHeader pathHeader {*reinterpret_cast<const BinaryChunkHeader*>(p)};
    auto headerSize = sizeof(BinaryChunkHeader);
    auto pathType = pathHeader.type;
    if(pathType == 'P')
    {
      auto pathSizeInBytes = pathHeader.dataBytes;
      const char * pChars = reinterpret_cast<const char*>(p + headerSize);

      return Path(TextFragment(pChars, pathSizeInBytes));
    }
    else
    {
      return Path();
    }
  }


  // Tree< Value >

  inline std::unique_ptr< std::vector<unsigned char> > valueTreeToBinary(Tree< Value > t)
  {
    std::vector<unsigned char> returnVector;

    // allocate group header
    returnVector.resize(sizeof(BinaryGroupHeader));

    // use iterator to serialize tree

    // NOTE: this resizes returnVector for each path/item, which is not ideal.
    // TODO each serializable object could have a getBinarySize() method so we can get the total size needed quickly and only allocate once.

    size_t elements{0};
    for(auto it = t.begin(); it != t.end(); ++it)
    {
      //std::cout << it.getCurrentNodePath() << " = (" << (*it).getTypeAsSymbol() << ") " << *it << "\n";

      Path p = it.getCurrentNodePath();
      Value v = (*it);

      // add path header

      auto binaryPath = pathToBinary(p);
      uint8_t* pathData = binaryPath->data();
      BinaryChunkHeader pathHeader {*reinterpret_cast<BinaryChunkHeader*>(pathData)};
      auto headerSize1 = sizeof(BinaryChunkHeader);
      uint8_t* pSrc1Start = pathData;
      auto dataSize1 = pathHeader.dataBytes;
      auto sizeToAdd = headerSize1 + dataSize1;
      uint8_t* pSrc1End = pathData + sizeToAdd;
      auto prevSize = returnVector.size();
      returnVector.resize(prevSize + sizeToAdd);
      uint8_t* pDest1 = returnVector.data() + prevSize;
      std::copy(pSrc1Start, pSrc1End, pDest1);

      //std::cout << " path @ "  << prevSize << ":" << (unsigned char)pathHeader.type << " " << pathHeader.dataBytes << " = " << p << "\n";

      // add value

      auto binaryValue = valueToBinary(v);
      uint8_t* valueData = binaryValue->data();
      BinaryChunkHeader testHeader {*reinterpret_cast<BinaryChunkHeader*>(valueData)};
      size_t headerSize2 = (testHeader.type == 'M') ? sizeof(BinaryMatrixHeader) : sizeof(BinaryChunkHeader);

      uint8_t* pSrc2Start = valueData;
      auto dataSize2 = testHeader.dataBytes;
      auto sizeToAdd2 = headerSize2 + dataSize2;
      uint8_t* pSrc2End = valueData + sizeToAdd2;
      auto prevSize2 = returnVector.size();
      returnVector.resize(prevSize2 + sizeToAdd2);
      uint8_t* pDest2 = returnVector.data() + prevSize2;
      std::copy(pSrc2Start, pSrc2End, pDest2);

      //std::cout << "value @ " << prevSize2 << ":" << (unsigned char)testHeader.type << " " << testHeader.dataBytes << " = " << v << "\n\n";

      elements++;
    }

    // write group header
    BinaryGroupHeader* pathHeader {reinterpret_cast<BinaryGroupHeader*>(returnVector.data())};
    pathHeader->elements = elements;

    //std::cout << "elements: " << elements << " total size: " << returnVector.size() << "\n";

    return ml::make_unique< std::vector<unsigned char> > ( returnVector );
  }

  inline Tree< Value > binaryToValueTree(const uint8_t* p)
  {
    Tree< Value > outputTree;
    const uint8_t* pData{p};

    // read group header
    BinaryGroupHeader pathHeader {*reinterpret_cast<const BinaryGroupHeader*>(p)};
    size_t elements = pathHeader.elements;
    // std::cout << "elements: " << elements << "\n";

    size_t idx{sizeof(BinaryGroupHeader)};
    for(int i=0; i<elements; ++i)
    {
      // read path chunk
      BinaryChunkHeader pathHeader {*reinterpret_cast<const BinaryChunkHeader*>(pData + idx)};
      auto pathType = pathHeader.type;
      auto pathSize = pathHeader.dataBytes;
      auto pathHeaderSize = sizeof(BinaryChunkHeader);
      auto path = binaryToPath(pData + idx);
      //std::cout << " path @ "  << idx << ":" << (unsigned char)pathType << " " << pathSize << " = " << binaryToPath(pData + idx) << "\n";
      idx += pathSize + pathHeaderSize;

      // read value chunk
      BinaryChunkHeader valueHeader {*reinterpret_cast<const BinaryChunkHeader*>(pData + idx)};
      auto valueType = valueHeader.type;
      auto valueSize = valueHeader.dataBytes;
      auto valueHeaderSize = (valueType == 'M') ? sizeof(BinaryMatrixHeader) : sizeof(BinaryChunkHeader);
      auto val = binaryToValue(pData + idx);
      outputTree[path] = val;
      // std::cout << "value @ " << idx << ":" << (unsigned char)valueType << " " << valueSize << " = " << binaryToValue(pData + idx) << "\n\n";
      idx += valueSize + valueHeaderSize;
    }

    return outputTree;
  }

} // namespace ml

