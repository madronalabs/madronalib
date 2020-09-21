
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#pragma once

#include <list>
#include <map>
#include <string>

#include "MLMatrix.h"
#include "MLPath.h"
#include "MLSymbol.h"
#include "MLText.h"

// Value: a modifiable property. Properties have four types: undefined, float,
// text, and matrix.

// TODO: instead of using Matrix directly here as a type, make a blob type
// and utilities (in Matrix) for conversion. This lets the current "model" code
// go into "app" because it doesn't depend on DSP math anymore, and increases
// reusability.

namespace ml
{
class Value
{
 public:
  static const Matrix nullSignal;

  enum Type
  {
    kUndefinedValue = 0,
    kFloatValue = 1,
    kTextValue = 2,
    kMatrixValue = 3
  };

  Value();
  Value(const Value& other);
  Value& operator=(const Value& other);
  Value(float v);
  Value(int v);
  Value(bool v);
  Value(long v);
  Value(double v);
  Value(const ml::Text& t);
  Value(const char* t);
  Value(const ml::Matrix& s);

  explicit operator bool() const { return mType != kUndefinedValue; }

  // matrix type constructor via initializer_list
  Value(std::initializer_list< float > values)
  {
    auto size = values.size();
    if (size == 0)
    {
      *this = Value();
    }
    else if (size == 1)
    {
      *this = Value(*values.begin());
    }
    else
    {
      *this = Value(Matrix(values));
    }
  }

  ~Value();

  inline const float getFloatValue() const { return mFloatVal; }

  inline const float getFloatValueWithDefault(float d) const
  {
    return (mType == kFloatValue) ? mFloatVal : d;
  }

  inline const float getBoolValue() const { return static_cast< bool >(mFloatVal); }

  inline const bool getBoolValueWithDefault(bool b) const
  {
    return (mType == kFloatValue) ? static_cast< bool >(mFloatVal) : b;
  }

  inline const float getIntValue() const { return static_cast< int >(mFloatVal); }

  inline const int getIntValueWithDefault(int d) const
  {
    return (mType == kFloatValue) ? static_cast< int >(mFloatVal) : d;
  }

  inline const ml::Text getTextValue() const
  {
    return (mType == kTextValue) ? (mTextVal) : ml::Text();
  }

  inline const ml::Text getTextValueWithDefault(Text d) const
  {
    return (mType == kTextValue) ? (mTextVal) : d;
  }

  inline const Matrix& getMatrixValue() const
  {
    return (mType == kMatrixValue) ? (mMatrixVal) : nullSignal;
  }

  inline const Matrix getMatrixValueWithDefault(Matrix d) const
  {
    return (mType == kMatrixValue) ? (mMatrixVal) : d;
  }

  // For each type of property, a setValue method must exist
  // to set the value of the property to that of the argument.
  //
  // For each type of property, if the size of the argument is equal to the
  // size of the current value, the value must be modified in place.
  // This guarantee keeps DSP graphs from allocating memory as they run.
  void setValue(const Value& v);
  void setValue(const float& v);
  void setValue(const int& v);
  void setValue(const bool& v);
  void setValue(const long& v);
  void setValue(const double& v);
  void setValue(const ml::Text& v);
  void setValue(const char* const v);
  void setValue(const Matrix& v);

  bool operator==(const Value& b) const;
  bool operator!=(const Value& b) const;

  Type getType() const { return mType; }
  Symbol getTypeAsSymbol() const
  {
    static Symbol kTypesAsSymbols[4]{"undefined", "float", "text", "matrix"};
    return kTypesAsSymbols[mType];
  }
  bool isUndefinedType() { return mType == kUndefinedValue; }
  bool isFloatType() { return mType == kFloatValue; }
  bool isTextType() { return mType == kTextValue; }
  bool isMatrixType() { return mType == kMatrixValue; }

  bool operator<<(const Value& b) const;

 private:
  // TODO reduce storage requirements and reduce copying!
  // -- this is a minimal-code start
  Type mType{kUndefinedValue};
  float mFloatVal{};
  ml::Text mTextVal{};
  Matrix mMatrixVal{};
};

// utilities

// note: this implementation does not disable this overload for array types
template < typename T, typename... Args >
std::unique_ptr< T > make_unique(Args&&... args)
{
  return std::unique_ptr< T >(new T(std::forward< Args >(args)...));
}

std::ostream& operator<<(std::ostream& out, const ml::Value& r);

}  // namespace ml
