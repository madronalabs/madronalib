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

#include "MLMatrix.h"
#include "MLSymbol.h"
#include "MLText.h"
#include "MLTextUtils.h"
#include "MLTree.h"
#include "MLValue.h"
#include "cJSON.h"
#include "MLSerialization.h"

namespace ml
{
struct BinaryGroupHeader
{
  size_t elements;
  size_t size;
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

TextFragment valueToText(const Value v)
{
  TextFragment t;
  switch (v.getType())
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
      // unimplemented
      return TextFragment{};  // {"M", matrixToText(v.getMatrixValue())};
      break;
    case Value::kUnsignedLongValue:
      return TextFragment{"L", textUtils::naturalNumberToText(v.getUnsignedLongValue())};
      break;

    // TODO blob!
  }
  return t;
}

Value textToValue(const Text v)
{
  return Value();  // TODO
}


std::vector< uint8_t > valueToBinary(Value v)
{
  std::vector< uint8_t > outputVector;
  auto headerSize = sizeof(BinaryChunkHeader);
  auto matrixHeaderSize = sizeof(BinaryMatrixHeader);
  
  switch (v.getType())
  {
    default:
    case Value::kUndefinedValue:
    {
      outputVector.resize(sizeof(BinaryChunkHeader));
      BinaryChunkHeader* header{reinterpret_cast<BinaryChunkHeader*>(outputVector.data())};
      *header = BinaryChunkHeader{'U', 0};
      break;
    }
    case Value::kFloatValue:
    {
      float f = v.getFloatValue();
      unsigned dataSize = sizeof(float);
      outputVector.resize(headerSize + dataSize);
      BinaryChunkHeader* header{reinterpret_cast<BinaryChunkHeader*>(outputVector.data())};
      *header = BinaryChunkHeader{'F', dataSize};
      float* pDest{reinterpret_cast<float*>(outputVector.data() + headerSize)};
      *pDest = f;
      break;
    }
    case Value::kTextValue:
    {
      auto textVal = v.getTextValue();
      size_t dataSize = textVal.lengthInBytes();
      
      // TODO safety, limits
      
      outputVector.resize(headerSize + dataSize);
      BinaryChunkHeader* header{reinterpret_cast<BinaryChunkHeader*>(outputVector.data())};
      *header = BinaryChunkHeader{'T', (unsigned int)dataSize};
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
      
      unsigned dataSize = width * height * depth * sizeof(float);
      
      // TODO safety, limits
      
      outputVector.resize(matrixHeaderSize + dataSize);
      BinaryMatrixHeader* header{reinterpret_cast<BinaryMatrixHeader*>(outputVector.data())};
      *header = BinaryMatrixHeader{'M', dataSize, width, height, depth};
      float* pDest{reinterpret_cast<float*>(outputVector.data() + matrixHeaderSize)};
      
      matrixVal.writeToPackedData(pDest);
      break;
    }
    case Value::kUnsignedLongValue:
    {
      auto f = v.getUnsignedLongValue();
      unsigned dataSize = sizeof(uint32_t);
      outputVector.resize(headerSize + dataSize);
      BinaryChunkHeader* header{reinterpret_cast<BinaryChunkHeader*>(outputVector.data())};
      *header = BinaryChunkHeader{'L', dataSize};
      uint32_t* pDest{reinterpret_cast<uint32_t*>(outputVector.data() + headerSize)};
      *pDest = f;
      break;
    }
    case Value::kBlobValue:
    {
      uint8_t* blobData = static_cast<uint8_t*>(v.getBlobData());
      unsigned int blobSize = (unsigned int)v.getBlobSize();
      outputVector.resize(headerSize + blobSize);
      BinaryChunkHeader* header{reinterpret_cast<BinaryChunkHeader*>(outputVector.data())};
      *header = BinaryChunkHeader{'B', blobSize};
      auto pDest{outputVector.data() + headerSize};
      std::copy(blobData, blobData + blobSize, pDest);
      break;
    }
  }
  return outputVector;
}

Value binaryToValue(const unsigned char* p)
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
    case 'M': // matrix
    {
      BinaryMatrixHeader header{*reinterpret_cast<const BinaryMatrixHeader*>(p)};
      const unsigned char* pData = p + sizeof(BinaryMatrixHeader);
      auto* pFloatData{reinterpret_cast<const float*>(pData)};
      Matrix m(header.width, header.height, header.depth);
      m.readFromPackedData(pFloatData);
      returnValue = Value{m};
      
      break;
    }
    case 'L': // long
    {
      const unsigned char* pData = p + sizeof(BinaryChunkHeader);
      auto* pLongData{reinterpret_cast<const uint32_t*>(pData)};
      uint32_t ul = *pLongData;
      returnValue = Value(ul);
      break;
    }
    case 'B': // blob
    {
      const unsigned char* pData = p + sizeof(BinaryChunkHeader);
      auto* pBlobData{reinterpret_cast<const void*>(pData)};
      returnValue = Value{pBlobData, header->dataBytes};
      break;
    }
  }
  return returnValue;
}

// float vector

std::vector< uint8_t > floatVectorToBinary(const std::vector<float>& inputVector)
{
  std::vector< uint8_t > outputVector;
  auto headerSize = sizeof(BinaryChunkHeader);
  auto matrixHeaderSize = sizeof(BinaryMatrixHeader);
  
  unsigned arrayDataSize = (unsigned)inputVector.size()*sizeof(float);
  outputVector.resize(headerSize + arrayDataSize);
  BinaryChunkHeader* header{reinterpret_cast<BinaryChunkHeader*>(outputVector.data())};
  *header = BinaryChunkHeader{'V', arrayDataSize};
  uint8_t* pDest{outputVector.data() + headerSize};
  const uint8_t* pSrc = reinterpret_cast<const uint8_t*>(inputVector.data());
  std::copy(pSrc, pSrc + arrayDataSize, pDest);

  return outputVector;
}


std::vector< float > binaryToFloatVector(const unsigned char* p)
{
  auto header{reinterpret_cast<const BinaryChunkHeader*>(p)};
  if (header->type == 'V')
  {
    const unsigned char* pData = p + sizeof(BinaryChunkHeader);
    const float* pVectorData{reinterpret_cast<const float*>(pData)};
    size_t vectorSize = header->dataBytes/sizeof(float);
    return std::vector< float >(pVectorData, pVectorData+vectorSize);
  }
    
  // on wrong type return empty vector
  return std::vector< float >();
}

// Path

std::vector<unsigned char> pathToBinary(Path p)
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

Path binaryDataToPath(const unsigned char* p)
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

Path binaryToPath(const std::vector<unsigned char>& p)
{
  return binaryDataToPath(p.data());
}

// Tree< Value >

std::vector<unsigned char> valueTreeToBinary(const Tree<Value>& t)
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
    // std::cout << it.getCurrentPath() << " = (" << (*it).getTypeAsSymbol()
    // << ") " << *it << "\n";
    
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
    
    auto binaryValue = valueToBinary(v);
    uint8_t* valueData = binaryValue.data();
    BinaryChunkHeader testHeader{*reinterpret_cast< BinaryChunkHeader* >(valueData)};
    size_t headerSize2 =
    (testHeader.type == 'M') ? sizeof(BinaryMatrixHeader) : sizeof(BinaryChunkHeader);
    
    uint8_t* pSrc2Start = valueData;
    auto dataSize2 = testHeader.dataBytes;
    auto sizeToAdd2 = headerSize2 + dataSize2;
    uint8_t* pSrc2End = valueData + sizeToAdd2;
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

Tree<Value> binaryToValueTree(const std::vector<unsigned char>& binaryData)
{
  Tree<Value> outputTree;
  const uint8_t* pData{binaryData.data()};
  
  if (binaryData.size() > sizeof(BinaryGroupHeader))
  {
    // read group header
    BinaryGroupHeader pathHeader{*reinterpret_cast<const BinaryGroupHeader*>(pData)};
    size_t elements = pathHeader.elements;
    size_t size = pathHeader.size;
    // std::cout << "elements: " << elements << " total size: " << size << "\n";
    
    if (binaryData.size() >= size)
    {
      size_t idx{sizeof(BinaryGroupHeader)};
      for (int i = 0; i < elements; ++i)
      {
        // read path chunk
        BinaryChunkHeader pathHeader{*reinterpret_cast<const BinaryChunkHeader*>(pData + idx)};
        // auto pathType = pathHeader.type;
        auto pathSize = pathHeader.dataBytes;
        auto pathHeaderSize = sizeof(BinaryChunkHeader);
        auto path = binaryDataToPath(pData + idx);
        // std::cout << " path @ "  << idx << ":" << (unsigned char)pathType <<
        // " " << pathSize << " = " << binaryToPath(pData + idx) << "\n";
        idx += pathSize + pathHeaderSize;
        
        // read value chunk
        BinaryChunkHeader valueHeader{*reinterpret_cast<const BinaryChunkHeader*>(pData + idx)};
        auto valueType = valueHeader.type;
        auto valueSize = valueHeader.dataBytes;
        auto valueHeaderSize =
        (valueType == 'M') ? sizeof(BinaryMatrixHeader) : sizeof(BinaryChunkHeader);
        auto val = binaryToValue(pData + idx);
        outputTree[path] = val;
        // std::cout << "value @ " << idx << ":" << (unsigned char)valueType <<
        // " " << valueSize << " = " << binaryToValue(pData + idx) << "\n\n";
        idx += valueSize + valueHeaderSize;
      }
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
  cJSON_AddItemToObject(pImpl->data, key.getText(), cJSON_CreateFloatArray(v.data(), v.size()));
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
      case Value::kUndefinedValue:
        break;
      case Value::kFloatValue:
        cJSON_AddNumberToObject(getData(root), keyStr, v.getFloatValue());
        break;
      case Value::kTextValue:
        cJSON_AddStringToObject(getData(root), keyStr, v.getTextValue().getText());
        break;
      case Value::kUnsignedLongValue:
        cJSON_AddNumberToObject(getData(root), keyStr, v.getUnsignedLongValue());
        break;
      case Value::kBlobValue:
      {
        uint8_t* blobData = static_cast<uint8_t*>(v.getBlobData());
        size_t blobSize = v.getBlobSize();
        std::vector<uint8_t> blobVec(blobData, blobData + blobSize);
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
          auto* pBlobData{reinterpret_cast<const void*>(blobDataVec.data())};
          auto blobValue = Value{pBlobData, blobDataVec.size()};
          r.add(newObjectPath, blobValue);
        }
        else
        {
          // convert ordinary strings into text
          r.add(newObjectPath, valueText);
        }
        break;
      }
      case cJSON_Object:
      {
        readJSONToValueTree(obj->child, r, newObjectPath, depth);
        break;
      }
      case cJSON_Array:
      {
        // read array at obj->child to blob
        std::vector< float > arrayElems;
        cJSON* array = obj->child;
        while(array->next)
        {
          // std::cout << ".";
          arrayElems.push_back((float)array->valuedouble);
          array = array->next;
        }
        Value arrayVal(arrayElems.data(), arrayElems.size()*sizeof(float));
        r.add(newObjectPath, arrayVal);
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
