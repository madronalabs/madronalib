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
class PluginController : public EditControllerEx1, public IMidiMapping
{
public:
  // create function required for Plug-in factory,
  // it will be called to create new instances of this controller
  static FUnknown* createInstance(void*) { return (IEditController*)new PluginController; }
  static FUID uid;
  
	PluginController();
	~PluginController();
	
  // IPluginBase interface
  tresult PLUGIN_API initialize(FUnknown* context) SMTG_OVERRIDE;
	tresult PLUGIN_API terminate() SMTG_OVERRIDE;
  
  // EditController interface
  tresult PLUGIN_API setComponentState(IBStream* state) SMTG_OVERRIDE;
  tresult PLUGIN_API notify(IMessage* message) SMTG_OVERRIDE;

  // IMidiMapping interface
  tresult PLUGIN_API getMidiControllerAssignment (int32 busIndex, int16 channel, CtrlNumber midiControllerNumber, ParamID& id) SMTG_OVERRIDE;
  
  OBJ_METHODS(PluginController, EditControllerEx1)
  DEFINE_INTERFACES
  DEF_INTERFACE(IMidiMapping)
  END_DEFINE_INTERFACES(EditControllerEx1)
  REFCOUNT_METHODS(EditControllerEx1)
  
};

}}} // namespaces
