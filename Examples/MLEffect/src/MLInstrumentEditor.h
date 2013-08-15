//
// MLInstrumentEditor.h
// Madrona Labs source code.
// Copyright 2010 Madrona Labs. All rights reserved.
//

#ifndef __EXAMPLEEDITOR_H__
#define __EXAMPLEEDITOR_H__

#include "MLTime.h"
#include "MLInstrumentBinaryData.h"
#include "MLInputProtocols.h"
#include "MLPluginEditor.h"
#include "MLInstrumentProcessor.h"
#include "MLAppBorder.h"
#include "MLInstrumentView.h"
#include "MLInstrumentController.h"

class MLInstrumentEditor :
	public MLPluginEditor
{
friend MLPluginEditor* CreateMLPluginEditor (MLPluginProcessor* const pProcessor, const MLRect& bounds, bool num, bool anim);

public:
	MLInstrumentEditor (MLPluginProcessor* const pProcessor);
    ~MLInstrumentEditor();
	void initialize (MLPluginProcessor* const pProcessor);
	
	void setWrapperFormat(int format);
	void resized ();

private:
	MLPluginProcessor* mpProcessor;
	MLAppBorder* mpBorder;
	MLInstrumentView* mpView;
	MLInstrumentController* mpController;

};

MLPluginEditor* CreateMLPluginEditor (MLPluginProcessor* const ownerProcessor, const MLRect& bounds, bool num, bool anim);


#endif // __EXAMPLEEDITOR_H__