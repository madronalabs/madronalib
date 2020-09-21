// TEMP - procs will just have a .cpp
// TODO make this a template

// work in progress - for testing only

#pragma once

#include "MLProcFactory.h"
#include "MLValue.h"

namespace ml
{
class ProcMultiply : public Proc
{
 public:
  static constexpr constStr paramNames[]{"a", "b", "c"};
  static constexpr constStr inputNames[]{"foo", "bar"};
  static constexpr constStr outputNames[]{"baz"};

  // Proc boilerplate
  static constexpr constStrArray pn_{paramNames};
  static constexpr constStrArray in_{inputNames};
  static constexpr constStrArray on_{outputNames};

  virtual const constStrArray& getParamNames() override { return pn_; }
  virtual const constStrArray& getInputNames() override { return in_; }
  virtual const constStrArray& getOutputNames() override { return on_; }

  // + 1 leaves room for setting without a branch when keys are not found.
  Value params[constCount(paramNames) + 1];
  DSPVector* inputs[constCount(inputNames) + 1];
  DSPVector* outputs[constCount(outputNames) + 1];

  inline Value& param(constStr str) { return params[constFind(paramNames, str)]; }

  inline DSPVector& input(constStr str) { return *inputs[constFind(inputNames, str)]; }

  inline DSPVector& output(constStr str) { return *outputs[constFind(outputNames, str)]; }

  // TODO remove and make non-const versions for external use
  // MLProc implementation (more boilerplate)
  virtual void setParam(constStr str, float v) override { params[constFind(paramNames, str)] = v; }
  void setInput(constStr str, DSPVector& v) override { inputs[constFind(inputNames, str)] = &v; }
  void setOutput(constStr str, DSPVector& v) override { outputs[constFind(outputNames, str)] = &v; }

  void process() override;
};

}  // namespace ml
