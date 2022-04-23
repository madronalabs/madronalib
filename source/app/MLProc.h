// madronalib: a C++ framework for DSP applications.
// Copyright (c) 2020 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

// new proc. work in progress!

#pragma once

#include "madronalib.h"
#include "mldsp.h"

namespace ml
{
// constexpr string courtesy of Scott Schurr
class constStr
{
 private:
  const char* const p_;
  const std::size_t size_;

 public:
  template <std::size_t N>
  constexpr constStr(const char (&a)[N]) : p_(a), size_(N - 1)
  {
  }

  constexpr char operator[](std::size_t n) const
  {
    return n < size_ ? p_[n] : throw std::out_of_range("");
  }

  constexpr std::size_t size() const { return size_; }

  constexpr const char* const data() const { return p_; }

  friend constexpr bool operator==(const constStr& a, const constStr& b);
};

constexpr bool constStringsEqual(char const* a, char const* b)
{
  return (*a && *b) ? (*a == *b && constStringsEqual(a + 1, b + 1)) : (!*a && !*b);
}

constexpr bool operator==(const constStr& a, const constStr& b)
{
  return constStringsEqual(a.p_, b.p_);
}

inline std::ostream& operator<<(std::ostream& out, const constStr& r)
{
  out << r.data();
  return out;
}

class constStrArray
{
 private:
  const constStr* const p_;
  const std::size_t size_;

 public:
  template <std::size_t N>
  constexpr constStrArray(const constStr (&a)[N]) : p_(a), size_(N)
  {
  }

  constexpr const constStr& operator[](std::size_t n) const
  {
    return n < size_ ? p_[n] : throw std::out_of_range("");
  }

  constexpr std::size_t size() const { return size_; }
};

// constexpr array count function
template <typename T, std::size_t N>
constexpr std::size_t constCount(T const (&)[N]) noexcept
{
  return N;
}

constexpr int constFindIndex(const constStr* begin, int i, int N, constStr const& value)
{
  return (!((i != N) && !(*begin == value))) ? (i) : constFindIndex(begin + 1, i + 1, N, value);
}

// returns the array length N if not found, otherwise the index of the array
// element equal to str.
template <std::size_t N>
constexpr int constFind(constStr const (&array)[N], constStr str)
{
  return constFindIndex(&(array[0]), 0, N, str);
}

// MLTEST proc class
// compiler needs to be able to query the functor / proc about its i/o size
// possibilities to turn bytecode into a list of process() calls

class Proc
{
 public:
  virtual ~Proc() = default;

  virtual void process() = 0;

  // These methods are the public interface to set a Proc's instance variables.
  // From inside a Proc subclass, the inline methods input(), output() and so on
  // should be used instead.

  // constStr methods
  virtual void setParam(constStr str, float f) = 0;
  virtual void setInput(constStr str, DSPVector& v) = 0;
  virtual void setOutput(constStr str, DSPVector& v) = 0;

  // the problem is the ambiguity between const / non-const methods.
  // are the const ones really needed from outside procs?
  // linear search seems fine. think uses.

  // make iterators and non-const find.

  /* TODO
  // Symbol methods
  inline void setParam(Symbol name, float val)
  {
          // find name, linear OK
          // set it
  }
  */

  virtual const constStrArray& getParamNames() = 0;
  virtual const constStrArray& getInputNames() = 0;
  virtual const constStrArray& getOutputNames() = 0;
};

}  // namespace ml
