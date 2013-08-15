//
// MLExampleEditor.h
// Madrona Labs source code.
// Copyright 2010 Madrona Labs. All rights reserved.
//

#ifndef __EXAMPLEEDITOR_H__
#define __EXAMPLEEDITOR_H__

#include "MLTime.h"
#include "MLExampleBinaryData.h"
#include "MLInputProtocols.h"
#include "MLPluginEditor.h"
#include "MLExampleProcessor.h"
#include "MLAppBorder.h"
#include "MLExampleView.h"
#include "MLExampleController.h"

class MLExampleEditor :
	public MLPluginEditor
{
friend MLPluginEditor* CreateMLPluginEditor (MLPluginProcessor* const pProcessor, const MLRect& bounds, bool num, bool anim);

public:
	MLExampleEditor (MLPluginProcessor* const pProcessor);
    ~MLExampleEditor();
	void initialize (MLPluginProcessor* const pProcessor);
	
	void setWrapperFormat(int format);
	void resized ();

private:
	MLPluginProcessor* mpProcessor;
	MLAppBorder* mpBorder;
	MLExampleView* mpView;
	MLExampleController* mpController;

};

MLPluginEditor* CreateMLPluginEditor (MLPluginProcessor* const ownerProcessor, const MLRect& bounds, bool num, bool anim);


#endif // __EXAMPLEEDITOR_H__