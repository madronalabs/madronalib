// madronalib: a C++ framework for DSP applications.
// Copyright (c) 2020 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#pragma once

#include "MLPath.h"
#include "MLValue.h"

namespace ml
{
struct Message final
{
  Path address{};
  Value value{};
  uint32_t flags{0};

  Message(Path h = Path(), Value v = Value(), uint32_t f = 0) : address(h), value(v), flags(f) {}
};

enum flags
{
  kMsgSequenceStart = 1 << 0,
  kMsgSequenceEnd = 1 << 1
};

// note: because std::vector will allocate on the fly, this implementation of
// MessageList is not safe for use in audio processing threads. Given the
// intended use in editors and controllers, this seems like a reasonable
// tradeoff.

using MessageList = std::vector<Message>;

}  // namespace ml

inline std::ostream& operator<<(std::ostream& out, const ml::Message& r)
{
  std::cout << "{" << r.address << ": " << r.value << "}";
  return out;
}
