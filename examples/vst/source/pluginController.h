// VST3 example code for madronalib
// (c) 2020, Madrona Labs LLC, all rights reserved
// see LICENSE.txt for details

#pragma once

#include "public.sdk/source/vst/vsteditcontroller.h"
#include "public.sdk/source/vst/vstparameters.h"
#include "pluginterfaces/vst/ivstmidicontrollers.h"
#include "pluginterfaces/base/ustring.h"

#include "pluginProcessor.h"

namespace Steinberg {
namespace Vst {
namespace llllpluginnamellll {

//-----------------------------------------------------------------------------
class TrackerController : public EditControllerEx1, public IMidiMapping
{
public:
  // create function required for Plug-in factory,
  // it will be called to create new instances of this controller
  static FUnknown* createInstance (void*) { return (IEditController*)new TrackerController; }
  static FUID uid;
  
	TrackerController ();
	~TrackerController ();
	
  // IPluginBase interface
  tresult PLUGIN_API initialize (FUnknown* context) SMTG_OVERRIDE;
	tresult PLUGIN_API terminate () SMTG_OVERRIDE;
  
  // EditController interface
  tresult PLUGIN_API setComponentState (IBStream* state) SMTG_OVERRIDE;
  tresult PLUGIN_API notify (IMessage* message) SMTG_OVERRIDE;
  tresult PLUGIN_API getMidiControllerAssignment (int32 busIndex, int16 channel, CtrlNumber midiControllerNumber, ParamID& tag/*out*/) SMTG_OVERRIDE;
  DELEGATE_REFCOUNT (EditControllerEx1)
  tresult PLUGIN_API queryInterface (const char* iid, void** obj) SMTG_OVERRIDE;
  
  // parameter IDs
  enum
  {
    kGainId = 0,  ///< for the gain value (is automatable)
    kBypassId    ///< Bypass value (we will handle the bypass process) (is automatable)
  };
};

}}} // namespaces
