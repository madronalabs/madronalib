// madronalib: a C++ framework for DSP applications.
// Copyright (c) 2020-2022 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

// converters to/from binary and text formats for various objects.

#pragma once

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <list>
#include <map>
#include <numeric>

#include "MLMatrix.h"
#include "MLSymbol.h"
#include "MLText.h"
#include "MLTextUtils.h"
#include "MLTree.h"
#include "MLValue.h"
#include "cJSON.h"

namespace ml
{

static TextFragment kBlobHeader("!BLOB!");

struct BinaryGroupHeader
{
  size_t elements;
  size_t size;
};

inline bool operator==(const BinaryGroupHeader& a, const BinaryGroupHeader& b)
{
  return ((a.elements == b.elements) && (a.size == b.size));
}

// with 0 elements, size can never be > 0, so this header will not be in any previous binaries.
static constexpr BinaryGroupHeader kBinaryGroupHeaderV2{0, 1};

struct BinaryChunkHeader
{
  unsigned int type : 8;
  unsigned int dataBytes : 24;
};

inline Value binaryToValue(const uint8_t* p)
{
  Value returnValue{};
  auto header{reinterpret_cast<const BinaryChunkHeader*>(p)};
  switch (header->type)
  {
    default:
    case 'U': // undefined
    {
      break;
    }
    case 'F': // float
    {
      const unsigned char* pData = p + sizeof(BinaryChunkHeader);
      auto* pFloatData{reinterpret_cast<const float*>(pData)};
      float f = *pFloatData;
      returnValue = Value{f};
      break;
    }
    case 'T': // text
    {
      const unsigned char* pData = p + sizeof(BinaryChunkHeader);
      auto* p{reinterpret_cast<const char*>(pData)};
      returnValue = Value{Text(p, header->dataBytes)};
      break;
    }
    case 'L': // long
    {
      const unsigned char* pData = p + sizeof(BinaryChunkHeader);
      auto* pLongData{reinterpret_cast<const uint32_t*>(pData)};
      int ul = *pLongData;
      returnValue = Value(ul);
      break;
    }
    case 'B': // blob
    {
      const uint8_t* pData = p + sizeof(BinaryChunkHeader);
      returnValue = Value(Blob(pData, header->dataBytes));
      break;
    }
  }
  return returnValue;
}

// Path

inline std::vector<unsigned char> pathToBinary(Path p)
{
  std::vector<unsigned char> outputVector;
  auto t = pathToText(p, '/');
  auto headerSize = sizeof(BinaryChunkHeader);
  auto dataSize = t.lengthInBytes();
  outputVector.resize(headerSize + dataSize);
  BinaryChunkHeader* header{reinterpret_cast<BinaryChunkHeader*>(outputVector.data())};
  header->type = 'P';
  header->dataBytes = (unsigned int)dataSize;
  auto pDest{outputVector.data() + headerSize};
  auto pSrc{t.getText()};
  std::copy(pSrc, pSrc + dataSize, pDest);
  return outputVector;
}

inline Path binaryToPath(const unsigned char* p)
{
  BinaryChunkHeader pathHeader{*reinterpret_cast<const BinaryChunkHeader*>(p)};
  auto headerSize = sizeof(BinaryChunkHeader);
  auto pathType = pathHeader.type;
  if (pathType == 'P')
  {
    auto pathSizeInBytes = pathHeader.dataBytes;
    const char* pChars = reinterpret_cast<const char*>(p + headerSize);
    return Path(TextFragment(pChars, pathSizeInBytes));
  }
  else
  {
    return Path();
  }
}

inline Path binaryToPath(const std::vector<unsigned char>& p) { return binaryToPath(p.data()); }

// Tree< Value >

inline std::vector<unsigned char> valueTreeToBinary(const Tree<Value>& t)
{
  std::vector<unsigned char> returnVector;
  
  // allocate group header
  returnVector.resize(sizeof(BinaryGroupHeader));
  
  // use iterator to serialize tree
  
  // NOTE: this resizes returnVector for each path/item, which is not ideal.
  // TODO each serializable object could have a getBinarySize() method so we can
  // get the total size needed quickly and only allocate once.
  
  
  size_t elements{0};
  for (auto it = t.begin(); it != t.end(); ++it)
  {
    Path p = it.getCurrentPath();
    Value v = (*it);
    
    // add path header
    
    auto binaryPath = pathToBinary(p);
    uint8_t* pathData = binaryPath.data();
    BinaryChunkHeader pathHeader{*reinterpret_cast<BinaryChunkHeader*>(pathData)};
    auto headerSize1 = sizeof(BinaryChunkHeader);
    uint8_t* pSrc1Start = pathData;
    auto dataSize1 = pathHeader.dataBytes;
    auto sizeToAdd = headerSize1 + dataSize1;
    uint8_t* pSrc1End = pathData + sizeToAdd;
    auto prevSize = returnVector.size();
    
    
    returnVector.resize(prevSize + sizeToAdd);
    uint8_t* pDest1 = returnVector.data() + prevSize;
    std::copy(pSrc1Start, pSrc1End, pDest1);
    
    // std::cout << " path @ "  << prevSize << ":" << (unsigned
    // char)pathHeader.type << " " << pathHeader.dataBytes << " = " << p <<
    // "\n";
    
    // add value
    
    auto dataBlob = v.getBlobValue();
    const uint8_t* valueData = reinterpret_cast<const uint8_t*>(dataBlob.data);
    const BinaryChunkHeader testHeader{*reinterpret_cast<const BinaryChunkHeader*>(valueData)};
    size_t headerSize2 = sizeof(BinaryChunkHeader);
    
    const uint8_t* pSrc2Start = valueData;
    auto dataSize2 = testHeader.dataBytes;
    auto sizeToAdd2 = headerSize2 + dataSize2;
    const uint8_t* pSrc2End = valueData + sizeToAdd2;
    auto prevSize2 = returnVector.size();
    
    
    returnVector.resize(prevSize2 + sizeToAdd2);
    uint8_t* pDest2 = returnVector.data() + prevSize2;
    std::copy(pSrc2Start, pSrc2End, pDest2);
    
    // std::cout << "value @ " << prevSize2 << ":" << (unsigned
    // char)testHeader.type << " " << testHeader.dataBytes << " = " << v <<
    // "\n\n";
    
    elements++;
  }
  
  // write group header
  BinaryGroupHeader* pathHeader{reinterpret_cast<BinaryGroupHeader*>(returnVector.data())};
  pathHeader->elements = elements;
  pathHeader->size = returnVector.size();
  
  // std::cout << "elements: " << elements << " total size: " <<
  // returnVector.size() << "\n";
  
  return returnVector;
}

// deprecated code maintained to read older binaries of patches etc.
inline Tree<Value> binaryToValueTreeOld(const std::vector<uint8_t>& binaryData)
{
  Tree<Value> outputTree;
  const uint8_t* pData{binaryData.data()};
  if (binaryData.size() > sizeof(BinaryGroupHeader))
  {
    BinaryGroupHeader groupHeader{*reinterpret_cast<const BinaryGroupHeader*>(pData)};
    size_t elements = groupHeader.elements;
    size_t size = groupHeader.size;
    if (binaryData.size() >= size)
    {
      size_t idx{sizeof(BinaryGroupHeader)};
      for (int i = 0; i < elements; ++i)
      {
        // get path
        BinaryChunkHeader pathHeader{*reinterpret_cast<const BinaryChunkHeader*>(pData + idx)};
        auto pathSize = pathHeader.dataBytes;
        auto pathHeaderSize = sizeof(BinaryChunkHeader);
        auto path = binaryToPath(pData + idx);
        idx += pathSize + pathHeaderSize;
        
        // get value
        BinaryChunkHeader valueHeader{*reinterpret_cast<const BinaryChunkHeader*>(pData + idx)};
        auto valueType = valueHeader.type;
        auto valueSize = valueHeader.dataBytes;
        auto valueHeaderSize = sizeof(BinaryChunkHeader);
        auto val = binaryToValue(pData + idx);
        idx += valueSize + valueHeaderSize;
        
        // write to tree
        outputTree[path] = val;
      }
    }
  }
  return outputTree;
}

inline Tree<Value> binaryToValueTreeNew(const std::vector<uint8_t>& binaryData)
{
  // write kBinaryGroupHeaderV2
  
  // write second header with actual info
  
  
}

inline Tree<Value> binaryToValueTree(const std::vector<uint8_t>& binaryData)
{
  Tree<Value> outputTree;
  const uint8_t* pData{binaryData.data()};
  size_t inputBytes = binaryData.size();
  
  if(inputBytes > sizeof(BinaryGroupHeader))
  {
    BinaryGroupHeader groupHeader{*reinterpret_cast<const BinaryGroupHeader*>(pData)};
    if(groupHeader == kBinaryGroupHeaderV2)
    {
      outputTree = binaryToValueTreeNew(binaryData);
    }
    else
    {
      outputTree = binaryToValueTreeOld(binaryData);
    }
  }
  return outputTree;
}


// make the cJSON interface usable with RAII style.
class JSONHolder
{
  cJSON _data;
  
public:
  JSONHolder()
  {
    memset(&_data, 0, sizeof(cJSON));
    _data.type = cJSON_Object;
  }
  
  JSONHolder(const JSONHolder& b) { _data = b._data; }
  
  JSONHolder(cJSON b) : _data(b) {}
  ~JSONHolder() { cJSON_Delete(_data.next); }
  
  cJSON* data() { return &_data; }
};


// return a JSON object representing the value tree. The caller is responsible
// for freeing the object.
//
// NOTE: this does not make the JSON tree, rather a flat structure with the
// path name for each object name! TODO fix
inline JSONHolder valueTreeToJSON(const Tree<Value>& t)
{
  JSONHolder root;
  
  for (auto it = t.begin(); it != t.end(); ++it)
  {
    Path p = it.getCurrentPath();
    TextFragment pathAsText(pathToText(p));
    Value v = (*it);
    
    const char* keyStr = pathAsText.getText();
    
    switch (v.getType())
    {
      case Value::kUndefined:
        break;
      case Value::kFloat:
        cJSON_AddNumberToObject(root.data(), keyStr, v.getFloatValue());
        break;
      case Value::kText:
        cJSON_AddStringToObject(root.data(), keyStr, v.getTextValue().getText());
        break;

      case Value::kBlob:
      {
        auto blob = v.getBlobValue();
        
        std::vector<uint8_t> blobVec(blob.data, blob.data + blob.size);
        TextFragment blobText(kBlobHeader, textUtils::base64Encode(blobVec));
        cJSON_AddStringToObject(root.data(), keyStr, blobText.getText());
        break;
      }
      default:
        // debug() << "MLAppState::saveStateToStateFile(): undefined param type! \n";
        break;
    }
  }
  return root;
}

inline Tree<Value> readJSONToValueTree(cJSON* obj, Tree<Value>& r, Path currentPath, int depth,
                                       bool isArray)
{
  int objIndex{0};
  while (obj)
  {
    Path newObjectPath;
    if (!isArray)
    {
      newObjectPath = Path(currentPath, Path(obj->string));
    }
    else
    {
      // TODO add array node markers so we can get back to the JSON original
      newObjectPath = Path(currentPath, Path(textUtils::naturalNumberToText(objIndex)));
    }
    
    switch (obj->type & 255)
    {
      case cJSON_Number:
      {
        float f(obj->valuedouble);
        r.add(newObjectPath, Value(f));
        break;
      }
      case cJSON_String:
      {
        TextFragment valueText(obj->valuestring);
        if(valueText.beginsWith(kBlobHeader)) // not wonderful
        {
          auto headerLen = kBlobHeader.lengthInCodePoints();
          auto textLen = valueText.lengthInCodePoints();
          auto body = textUtils::subText(valueText, headerLen, textLen);
          
          auto blobDataVec = textUtils::base64Decode(body.getText());
          r.add(newObjectPath, Value(blobDataVec));
        }
        else
        {
          r.add(newObjectPath, Value(valueText));
        }
        break;
      }
      case cJSON_Object:
      {
        readJSONToValueTree(obj->child, r, newObjectPath, depth, false);
        break;
      }
      case cJSON_Array:
      {
        readJSONToValueTree(obj->child, r, newObjectPath, depth, true);
        break;
      }
      default:
        break;
    }
    obj = obj->next;
    objIndex++;
  }
  
  return r;
}

inline Tree<Value> JSONToValueTree(JSONHolder root)
{
  Tree<Value> r;
  if (root.data())
  {
    cJSON* obj = root.data()->child;
    return readJSONToValueTree(obj, r, "", 0, false);
  }
  return r;
}

inline TextFragment JSONToText(JSONHolder root) { return TextFragment(cJSON_Print(root.data())); }

inline JSONHolder textToJSON(TextFragment t)
{
  cJSON* root = cJSON_Parse(t.getText());
  if(root)
  {
    return JSONHolder(*root);
  }
  else
  {
    return JSONHolder();
  }
}

}  // namespace ml
