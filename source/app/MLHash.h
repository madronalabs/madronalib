
// madronalib: a C++ framework for DSP applications.
// Copyright (c) 2020-2022 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#pragma once

namespace ml
{

// hashing: 64-bit FNV-1a

namespace fnvConsts
{
constexpr uint64_t k1{14695981039346656037ull};
constexpr uint64_t k2{1099511628211ull};
}  // namespace fnvConsts

// compile-time version

namespace detail
{
constexpr uint64_t fnv1aSubstring(uint64_t h, const char* s, size_t len)
{
  return (len == 0)
             ? h
             : fnv1aSubstring((h ^ static_cast<uint64_t>(*s)) * fnvConsts::k2, s + 1, len - 1);
}
}  // namespace detail

constexpr uint64_t fnv1aSubstring(const char* s, size_t len)
{
  return detail::fnv1aSubstring(fnvConsts::k1, s, len);
}

// Runtime version for dynamic strings

inline uint64_t fnv1aRuntime(const char* str, size_t n)
{
  uint64_t hash = fnvConsts::k1;
  for (size_t i = 0; i < n; ++i)
  {
    hash = (hash ^ static_cast<uint64_t>(str[i])) * fnvConsts::k2;
  }
  return hash;
}

inline uint64_t fnv1aRuntime(const char* str) { return fnv1aRuntime(str, strlen(str)); }

// the main hashing function for string literals, used in for example case(hash("foo"))

template <size_t N>
constexpr uint64_t hash(const char (&sym)[N])
{
  return fnv1aSubstring(sym, N - 1);
}

}  // namespace ml
