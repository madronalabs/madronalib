/*
 *  mdaTrackerProcessor.h
 *  mda-vst3
 *
 *  Created by Arne Scheffler on 6/14/08.
 *
 *  mda VST Plug-ins
 *
 *  Copyright (c) 2008 Paul Kellett
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#pragma once

#include "mdaBaseProcessor.h"

namespace Steinberg {
namespace Vst {
namespace mda {

//-----------------------------------------------------------------------------
class TrackerProcessor : public AudioEffect
{
public:
  TrackerProcessor ();
  ~TrackerProcessor ();
  
  void allocParameters (int32 _numParams);
  
  virtual void setBypass (bool state, int32 sampleOffset);
  virtual void setParameter (ParamID index, ParamValue newValue, int32 sampleOffset);
  
  virtual bool hasProgram () const { return false; }
  virtual uint32 getCurrentProgram () const { return 0; }
  virtual void setCurrentProgram (uint32 val) {}
  virtual void setCurrentProgramNormalized (ParamValue val) {}
  
  bool processParameterChanges (IParameterChanges* changes);
  
  tresult PLUGIN_API setBusArrangements (SpeakerArrangement* inputs, int32 numIns, SpeakerArrangement* outputs, int32 numOuts) SMTG_OVERRIDE;

  tresult PLUGIN_API initialize (FUnknown* context) SMTG_OVERRIDE;
  tresult PLUGIN_API process (ProcessData& data) SMTG_OVERRIDE;
  tresult PLUGIN_API terminate () SMTG_OVERRIDE;
  tresult PLUGIN_API setActive (TBool state) SMTG_OVERRIDE;
  tresult PLUGIN_API setState (IBStream* state) SMTG_OVERRIDE;
  tresult PLUGIN_API getState (IBStream* state) SMTG_OVERRIDE;

  void doProcessing (ProcessData& data) ;
  
  //-----------------------------------------------------------------------------
  static FUnknown* createInstance (void*) { return (IAudioProcessor*)new TrackerProcessor; }
  static FUID uid;
  //-----------------------------------------------------------------------------
  float wet;
  
  ParamValue* params{0};
  size_t numParams{0};
  
  bool bypassState{false};
};

}}} // namespaces

