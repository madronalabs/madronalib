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

constexpr unsigned int kPathType{'P'};

struct BinaryChunkHeader
{
  unsigned int type : 8;
  unsigned int dataBytes : 24;
};

// Path

inline size_t getBinarySize(Path p)
{
  auto t = pathToText(p, '/');
  auto headerSize = sizeof(BinaryChunkHeader);
  auto dataSize = t.lengthInBytes();
  return headerSize + dataSize;
}

inline Path readPathFromBinary(const uint8_t*& readPtr)
{
  Path r;
  auto headerSize = sizeof(BinaryChunkHeader);
  size_t pathSize;
  BinaryChunkHeader pathHeader{*reinterpret_cast<const BinaryChunkHeader*>(readPtr)};

  auto pathType = pathHeader.type;
  if (pathType == 'P')
  {
    pathSize = pathHeader.dataBytes;
    const char* pChars = reinterpret_cast<const char*>(readPtr + headerSize);
    r = Path(TextFragment(pChars, pathSize));
  }
  readPtr += (headerSize + pathSize);
  return r;
}

// write the binary representation of the Path and increment the destination pointer.
inline void writeBinaryRepresentation(const Path& p, uint8_t*& writePtr)
{
  auto t = pathToText(p, '/');
  auto headerSize = sizeof(BinaryChunkHeader);
  unsigned int dataSize = t.lengthInBytes();
  
  // write header
  BinaryChunkHeader header{kPathType, dataSize};
  memcpy(writePtr, &header, headerSize);
  writePtr += headerSize;
  
  // data
  const uint8_t* textData = (uint8_t *)t.getText();
  memcpy(writePtr, textData, dataSize);
  writePtr += dataSize;
}

// Tree< Value >

inline std::vector<unsigned char> valueTreeToBinary(const Tree<Value>& t)
{
  std::vector<uint8_t> returnVector;
  constexpr size_t headerSize = sizeof(BinaryGroupHeader);
  
  // calculate size
  size_t totalSize{sizeof(BinaryGroupHeader)};
  for (auto it = t.begin(); it != t.end(); ++it)
  {
    totalSize += getBinarySize(it.getCurrentPath());
    totalSize += getBinarySize(*it);
  }
  totalSize += headerSize*2;
  returnVector.resize(totalSize);
  
  // advance past two headers, which we will fill in later
  uint8_t* writePtr = returnVector.data() + headerSize*2;
  
  // use iterator to serialize tree
  size_t elements{0};
  for (auto it = t.begin(); it != t.end(); ++it)
  {
    // add path
    writeBinaryRepresentation(it.getCurrentPath(), writePtr);
    
    // add value
    ValueUtils::writeBinaryRepresentation((*it), writePtr);
        
    elements++;
  }
  
  // write version header
  writePtr = returnVector.data();
  BinaryGroupHeader* versionHeader{reinterpret_cast<BinaryGroupHeader*>(writePtr)};
  *versionHeader = kBinaryGroupHeaderV2;
  writePtr += headerSize;

  // write main header
  BinaryGroupHeader* mainHeader{reinterpret_cast<BinaryGroupHeader*>(writePtr)};
  mainHeader->elements = elements;
  mainHeader->size = returnVector.size();

  return returnVector;
}

inline Tree<Value> binaryToValueTreeNew(const std::vector<uint8_t>& binaryData)
{
  Tree<Value> outputTree;
  size_t inputSize;
  constexpr size_t headerSize = sizeof(BinaryGroupHeader);

  if(inputSize > headerSize*2)
  {
    const uint8_t* readPtr = binaryData.data();
    readPtr += headerSize;
    auto mainHeader{reinterpret_cast<const BinaryGroupHeader*>(readPtr)};
    auto elements = mainHeader->elements;
    auto totalSize = mainHeader->size;
    
    if(binaryData.size() >= totalSize)
    {
      readPtr += headerSize;
      for (int i = 0; i < elements; ++i)
      {
        auto path = readPathFromBinary(readPtr);
        outputTree[path] = ValueUtils::readBinaryRepresentation(readPtr);
      }
    }
  }
  return outputTree;
}

// deprecated code maintained for now to read older binaries of patches etc.

inline Path binaryToPathOld(const uint8_t* p)
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

inline Value binaryToValueOld(const uint8_t* p)
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
        auto path = binaryToPathOld(pData + idx);
        idx += pathSize + pathHeaderSize;
        
        // get value
        BinaryChunkHeader valueHeader{*reinterpret_cast<const BinaryChunkHeader*>(pData + idx)};
        auto valueType = valueHeader.type;
        auto valueSize = valueHeader.dataBytes;
        auto valueHeaderSize = sizeof(BinaryChunkHeader);
        auto val = binaryToValueOld(pData + idx);
        idx += valueSize + valueHeaderSize;
        
        // write to tree
        outputTree[path] = val;
      }
    }
  }
  return outputTree;
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


// return a human-readable JSON object representing the value tree. The caller is responsible
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
