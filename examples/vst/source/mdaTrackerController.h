/*
 *  mdaTrackerController.h
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

#include "public.sdk/source/vst/vsteditcontroller.h"
#include "public.sdk/source/vst/vstparameters.h"
#include "pluginterfaces/vst/ivstmidicontrollers.h"
#include "pluginterfaces/base/ustring.h"

#include "mdaParameter.h"
#include "mdaTrackerProcessor.h"

namespace Steinberg {
namespace Vst {
namespace ml {

//-----------------------------------------------------------------------------
class TrackerController : public EditControllerEx1, public IMidiMapping
{
public:
  // create function required for Plug-in factory,
  // it will be called to create new instances of this controller
  static FUnknown* createInstance (void*) { return (IEditController*)new TrackerController; }

	TrackerController ();
	~TrackerController ();
	
  //---from IPluginBase--------
  tresult PLUGIN_API initialize (FUnknown* context) SMTG_OVERRIDE;
	tresult PLUGIN_API terminate () SMTG_OVERRIDE;
  
  //---from EditController-----
  tresult PLUGIN_API setComponentState (IBStream* state) SMTG_OVERRIDE;


	tresult PLUGIN_API getParamStringByValue (ParamID tag, ParamValue valueNormalized, String128 string) SMTG_OVERRIDE;
	tresult PLUGIN_API getParamValueByString (ParamID tag, TChar* string, ParamValue& valueNormalized) SMTG_OVERRIDE;

//-----------------------------------------------------------------------------
	static FUID uid;
  
  

  tresult PLUGIN_API notify (IMessage* message) SMTG_OVERRIDE;
   
  tresult PLUGIN_API getMidiControllerAssignment (int32 busIndex, int16 channel, CtrlNumber midiControllerNumber, ParamID& tag/*out*/) SMTG_OVERRIDE;
  
  ParameterContainer& getParameters () { return EditController::parameters; }
  
  //-----------------------------
  DELEGATE_REFCOUNT (EditControllerEx1)
  tresult PLUGIN_API queryInterface (const char* iid, void** obj) SMTG_OVERRIDE;
  //-----------------------------
  
  
  //-----------------------------------------------------------------------------
  // parameter IDs
  enum
  {
    kGainId = 0,  ///< for the gain value (is automatable)
    kVuPPMId,    ///< for the Vu value return to host (ReadOnly parameter for our UI)
    kBypassId    ///< Bypass value (we will handle the bypass process) (is automatable)
  };

  /*
  enum {
    kMagicNumber = 9999999,
    kBypassParam = 'bpas',
    kPresetParam = 'prst',
    kModWheelParam = 'modw',
    kBreathParam = 'brth',
    kCtrler3Param = 'ct03',
    kExpressionParam = 'expr',
    kPitchBendParam = 'pitb',
    kSustainParam = 'sust',
    kAftertouchParam = 'aftt',
  };
*/
  
protected:
  double getSampleRate () const { return sampleRate; }
  int32 midiCCParamID[kCountCtrlNumber];
  double sampleRate;

};

}}} // namespaces
