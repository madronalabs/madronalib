// madronalib: a C++ framework for DSP applications.
// Copyright (c) 2020-2022 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

// converters to/from binary and text formats for various objects.

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
#include "MLSerialization.h"
#include "MLMemoryUtils.h"

namespace ml
{

struct BinaryGroupHeader
{
  size_t elements;
  size_t size;
};

bool operator==(const BinaryGroupHeader& a, const BinaryGroupHeader& b)
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
  BinaryChunkHeader(int t, size_t bytes)
  {
    type = t;
    dataBytes = static_cast<unsigned int>(bytes)&0x00FFFFFF;
  }
};


// Values

struct ValueBinaryHeader
{
  static constexpr int kTypeBits{4};
  
  // type of value
  unsigned int type : kTypeBits;
  
  // size of data, not including header
  unsigned int size : Value::kMaxDataSizeBits;
};

static_assert((2 << ValueBinaryHeader::kTypeBits) >= Value::kNumTypes);

size_t getBinarySize(const Value& v)
{
  return sizeof(ValueBinaryHeader) + v.size();
}

std::vector<uint8_t> valueToBinary(Value v)
{
  // allocate vector and setup pointer
  std::vector<uint8_t> result;
  result.resize(getBinarySize(v));
  uint8_t* writePtr = result.data();
  
  // write header
  ValueBinaryHeader header{v.getType(), v.size()};
  memcpy(writePtr, &header, sizeof(ValueBinaryHeader));
  writePtr += sizeof(ValueBinaryHeader);
  
  // write data
  memcpy(writePtr, v.data(), v.size());
  return result;
}

void writeValueToBinary(Value v, uint8_t*& writePtr)
{
  // write header
  ValueBinaryHeader header{v.getType(), v.size()};
  memcpy(writePtr, &header, sizeof(ValueBinaryHeader));
  writePtr += sizeof(ValueBinaryHeader);
  
  // write data
  memcpy(writePtr, v.data(), v.size());
  writePtr += v.size();
}

Value readBinaryToValue(const uint8_t*& readPtr)
{
  // read header
  ValueBinaryHeader header;
  memcpy(&header, readPtr, sizeof(ValueBinaryHeader));
  readPtr += sizeof(ValueBinaryHeader);
  
  // copy readPtr at start of data, advance parameter ptr
  const uint8_t* dataPtr = readPtr;
  readPtr += header.size;
  
  // Use the private constructor via friend access
  return Value(header.type, header.size, dataPtr);
}

Value binaryToValue(const std::vector<uint8_t>& dataVec)
{
  const uint8_t* readPtr = dataVec.data();
  
  // Use the private constructor via friend access
  return readBinaryToValue(readPtr);
}


// Paths

size_t getBinarySize(Path p)
{
  auto t = pathToText(p, '/');
  auto headerSize = sizeof(BinaryChunkHeader);
  auto dataSize = t.lengthInBytes();
  return headerSize + dataSize;
}

Path readPathFromBinary(const uint8_t*& readPtr)
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
void writeBinaryRepresentation(const Path& p, uint8_t*& writePtr)
{
  auto t = pathToText(p, '/');
  auto headerSize = sizeof(BinaryChunkHeader);
  auto dataSize = t.lengthInBytes();
  
  // write header
  BinaryChunkHeader header{kPathType, (unsigned int)dataSize};
  memcpy(writePtr, &header, headerSize);
  writePtr += headerSize;
  
  // data
  const uint8_t* textData = (uint8_t *)t.getText();
  memcpy(writePtr, textData, dataSize);
  writePtr += dataSize;
}


// Tree< Value >

std::vector<unsigned char> valueTreeToBinary(const Tree<Value>& t)
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
    writeValueToBinary((*it), writePtr);
    
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

Tree<Value> binaryToValueTreeNew(const std::vector<uint8_t>& binaryData)
{
  Tree<Value> outputTree;
  const size_t inputSize = binaryData.size();
  constexpr size_t headerSize = sizeof(BinaryGroupHeader);
  
  if(inputSize > headerSize*2)
  {
    const uint8_t* readPtr = binaryData.data();
    readPtr += headerSize;
    auto mainHeader{reinterpret_cast<const BinaryGroupHeader*>(readPtr)};
    auto elements = mainHeader->elements;
    auto totalSize = mainHeader->size;
    
    if(inputSize >= totalSize)
    {
      readPtr += headerSize;
      for (int i = 0; i < elements; ++i)
      {
        auto path = readPathFromBinary(readPtr);
        outputTree[path] = readBinaryToValue(readPtr);
      }
    }
  }
  return outputTree;
}


// deprecated code maintained for now to read older binaries of patches etc.

Path binaryToPathOld(const uint8_t* p)
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

Value binaryToValueOld(const uint8_t* p)
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
      returnValue = Value(f);
      break;
    }
    case 'T': // text
    {
      const unsigned char* pData = p + sizeof(BinaryChunkHeader);
      auto* p{reinterpret_cast<const char*>(pData)};
      returnValue = Value(Text(p, header->dataBytes));
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
      returnValue = Value(pData, header->dataBytes);
      break;
    }
  }
  return returnValue;
}

Tree<Value> binaryToValueTreeOld(const std::vector<uint8_t>& binaryData)
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

Tree<Value> binaryToValueTree(const std::vector<uint8_t>& binaryData)
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


// JSONHolder

struct JSONHolder::Impl
{
  cJSON* data;
};

JSONHolder::JSONHolder()
{
  pImpl = new Impl;
  pImpl->data = (cJSON*)malloc(sizeof(cJSON));  // Use malloc for cJSON compatibility
  memset(pImpl->data, 0, sizeof(cJSON));
  pImpl->data->type = cJSON_Object;
}

JSONHolder::~JSONHolder()
{
  if (pImpl)
  {
    if (pImpl->data)
    {
      cJSON_Delete(pImpl->data);
    }
    delete pImpl;
    pImpl = nullptr;
  }
}

JSONHolder::JSONHolder(JSONHolder&& other) noexcept : pImpl(other.pImpl)
{
  other.pImpl = nullptr;
}

JSONHolder& JSONHolder::operator=(JSONHolder&& other) noexcept
{
  if (this != &other)
  {
    // Clean up our current data
    if (pImpl)
    {
        if (pImpl->data)
        {
            cJSON_Delete(pImpl->data);
        }
        delete pImpl;  
    }

    // Transfer ownership
    pImpl = other.pImpl;
    other.pImpl = nullptr; 
  }
  return *this;
}

void JSONHolder::addNumber(TextFragment key, double number)
{
  cJSON_AddNumberToObject(pImpl->data, key.getText(), number);
}

void JSONHolder::addString(TextFragment key, const char* str)
{
  cJSON_AddStringToObject(pImpl->data, key.getText(), str);
}

void JSONHolder::addFloatVector(TextFragment key, std::vector<float>& v)
{
  cJSON_AddItemToObject(pImpl->data, key.getText(), cJSON_CreateFloatArray(v.data(), sizeToInt(v.size())));
}

void JSONHolder::addJSON(TextFragment key, JSONHolder& j)
{
  // Transfer the entire cJSON tree to this object
  cJSON_AddItemToObject(pImpl->data, key.getText(), j.pImpl->data);

  // j no longer owns its data - set to nullptr so destructor won't try to delete
  j.pImpl->data = nullptr;
}

cJSON* getData(const JSONHolder& cj)
{
  return cj.pImpl->data;
}

void setData(JSONHolder& cj, cJSON* pData)
{
  cj.pImpl->data = pData;
}

// return a JSON object representing the value tree. The caller is responsible
// for freeing the object.
//
// NOTE: this does not make the JSON tree, rather a flat structure with the
// path name for each object name! TODO fix
JSONHolder valueTreeToJSON(const Tree<Value>& t)
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
        cJSON_AddNumberToObject(getData(root), keyStr, v.getFloatValue());
        break;
      case Value::kFloatArray:
      {
        auto a = cJSON_CreateFloatArray(v.getFloatArrayPtr(), sizeToInt(v.getFloatArraySize()));
        if(a)
        {
          cJSON_AddItemToObject(getData(root), keyStr, a);
        }
        break;
      }
      case Value::kText:
        cJSON_AddStringToObject(getData(root), keyStr, v.getTextValue().getText());
        break;
      case Value::kInt:
        cJSON_AddNumberToObject(getData(root), keyStr, v.getIntValue());
        break;
      case Value::kBlob:
      {
        std::vector<uint8_t> blobVec(v.data(), v.data() + v.size());
        TextFragment blobText(kBlobHeader, textUtils::base64Encode(blobVec));
        cJSON_AddStringToObject(getData(root), keyStr, blobText.getText());
        break;
      }
      default:
        // debug() << "MLAppState::saveStateToStateFile(): undefined param type! \n";
        break;
    }
  }
  return root;
}

void readJSONToValueTree(cJSON* obj, Tree< Value >& r, Path currentPath, int depth)
{
  int objIndex{0};

  while (obj)
  {
    Path newObjectPath(currentPath, Path(obj->string));
    switch (obj->type & 255)
    {
      case cJSON_Number:
      {
        r.add(newObjectPath, (float)obj->valuedouble);
        break;
      }
      case cJSON_String:
      {
        TextFragment valueText(obj->valuestring);

        if(valueText.beginsWith(kBlobHeader))
        {
          // convert strings starting with the header into Blobs
          auto headerLen = kBlobHeader.lengthInCodePoints();
          auto textLen = valueText.lengthInCodePoints();
          auto body = textUtils::subText(valueText, headerLen, textLen);
          
          auto blobDataVec = textUtils::base64Decode(body.getText());
          r.add(newObjectPath, Value(blobDataVec.data(), blobDataVec.size()));
        }
        else
        {
          // convert ordinary strings into text
          r.add(newObjectPath, Value(valueText));
        }
        break;
      }
      case cJSON_Object:
      {
        // we only recurse for entire objects.
        readJSONToValueTree(obj->child, r, newObjectPath, depth);
        break;
      }
      case cJSON_Array:
      {
        std::vector< float > arrayElems;
        cJSON *c = obj->child;
        while(c)
        {
          arrayElems.push_back((float)c->valuedouble);
          c = c->next;
        }
        r.add(newObjectPath, Value(arrayElems));
        break;
      }
      default:
      {
        break;
      }
    }
    obj = obj->next;
    objIndex++;
  }
}

Tree<Value> JSONToValueTree(const JSONHolder& root)
{
  Tree< Value > r;
  if (getData(root))
  {
    cJSON* obj = getData(root)->child;
    readJSONToValueTree(obj, r, "", 0);
  }
  return r;
}

JSONHolder textToJSON(TextFragment t)
{
  JSONHolder root;
  cJSON* cjp = cJSON_Parse(t.getText());

  if(cjp)
  {
    cJSON_Delete(root.pImpl->data);
    root.pImpl->data = cjp;
  }
  
  return root;
}

TextFragment JSONToText(const JSONHolder& root)
{
  char* jsonString = cJSON_Print(getData(root));
  TextFragment result(jsonString);
  free(jsonString);  // Free the malloc'd string
  return result;
}


}  // namespace ml
