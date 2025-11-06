//
// Created by Randy Jones on 2/25/25.
//

#include "MLEvent.h"

namespace ml
{

const char* Event::typeNames[kNumEventTypes] = {"NUL", "ON ", "RET", "SUS", "OFF", "PED",
                                                "CC ", "BND", "NPR", "CPR", "PGM"};

}  // namespace ml
