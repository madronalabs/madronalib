
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#pragma once

#include <string>
#include <list>
#include <map>

#include "MLMatrix.h"
#include "MLSymbol.h"
#include "MLText.h"
#include "MLPath.h"

// Value: a modifiable property. Properties have four types: undefined, float, text, and signal.

// TODO: instead of using Matrix directly here as a type, make a blob type
// and utilities (in Matrix) for conversion. This lets the current "model" code go into "app"
// because it doesn't depend on DSP math anymore, and increases reusability.

namespace ml{

  class Value
  {
  public:
    static const Matrix nullSignal;

    enum Type
    {
      kUndefinedValue  = 0,
      kFloatValue  = 1,
      kTextValue = 2,
      kMatrixValue = 3
    };

    Value();
    Value(const Value& other);
    Value& operator= (const Value & other);
    Value(float v);
    Value(int v);
    Value(long v);
    Value(double v);
    Value(const ml::Text& t);
    Value(const char* t);
    Value(const ml::Matrix& s);

    // signal type constructor via initializer_list
    Value (std::initializer_list<float> values)
    {
      *this = Value(Matrix(values));
    }

    ~Value();

    inline const float getFloatValue() const
    {
      return mFloatVal;
      // static const float nullFloat = 0.f;
      // return (mType == kFloatValue) ? mFloatVal : nullFloat;
    }

    inline const ml::Text getTextValue() const
    {
      return (mType == kTextValue) ? (mTextVal) : ml::Text();
    }

    inline const Matrix& getSignalValue() const
    {
      return (mType == kMatrixValue) ? (mSignalVal) : nullSignal;
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
    void setValue(const long& v);
    void setValue(const double& v);
    void setValue(const ml::Text& v);
    void setValue(const char* const v);
    void setValue(const Matrix& v);

    bool operator== (const Value& b) const;
    bool operator!= (const Value& b) const;

    Type getType() const { return mType; }
    Symbol getTypeAsSymbol() const
    {
      static Symbol kTypesAsSymbols[4] {"undefined", "float", "text", "matrix"};
      return kTypesAsSymbols[mType];
    }
    bool isUndefinedType(){ return mType == kUndefinedValue; }
    bool isFloatType(){ return mType == kFloatValue; }
    bool isTextType(){ return mType == kTextValue; }
    bool isMatrixType(){ return mType == kMatrixValue; }

    bool operator<< (const Value& b) const;

  private:
    // TODO reduce storage requirements and reduce copying!
    // -- this is a minimal-code start
    Type mType;
    float mFloatVal;
    ml::Text mTextVal;
    Matrix mSignalVal;
  };


  // utilities

  struct ValueChange
  {
    ml::Path name;

    // note: the order of the values is important for creating Values from initializer lists.
    Value newValue;
    Value oldValue;

    explicit operator bool() const
    {
      return ((oldValue == Value()) && (newValue == Value()));
    }
  };

  // note: this implementation does not disable this overload for array types
  template<typename T, typename... Args>
  std::unique_ptr<T> make_unique(Args&&... args)
  {
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
  }

  // Define a type for initializing a new object with a list of ValueChanges.
  // The name is potentially misleading but reads clearly. This is really only needed
  // in concert with make_unique, thus the underscore.
  using with_values = std::initializer_list<ValueChange>;

} // namespace ml

std::ostream& operator<< (std::ostream& out, const ml::Value & r);

