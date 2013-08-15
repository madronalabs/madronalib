//
// MLDemoInstrumentEditor.h
// Madrona Labs source code.
// Copyright 2010 Madrona Labs. All rights reserved.
//

#ifndef __EXAMPLEEDITOR_H__
#define __EXAMPLEEDITOR_H__

#include "MLTime.h"
#include "MLDemoInstrumentBinaryData.h"
#include "MLInputProtocols.h"
#include "MLPluginEditor.h"
#include "MLDemoInstrumentProcessor.h"
#include "MLAppBorder.h"
#include "MLDemoInstrumentView.h"
#include "MLDemoInstrumentController.h"

class MLDemoInstrumentEditor :
	public MLPluginEditor

// TEST
    , public SliderListener


{
friend MLPluginEditor* CreateMLPluginEditor (MLPluginProcessor* const pProcessor, const MLRect& bounds, bool num, bool anim);

public:
	MLDemoInstrumentEditor (MLPluginProcessor* const pProcessor);
    ~MLDemoInstrumentEditor();
	void initialize (MLPluginProcessor* const pProcessor);
	
	void setWrapperFormat(int format);
	void resized ();

    // TEST
    void sliderValueChanged (Slider*);

private:
	MLPluginProcessor* mpProcessor;
	MLAppBorder* mpBorder;
	MLDemoInstrumentView* mpView;
	MLDemoInstrumentController* mpController;
    
    // TEST
    Slider* pGainSlider;
    MLDial* pSlider2;


};

MLPluginEditor* CreateMLPluginEditor (MLPluginProcessor* const ownerProcessor, const MLRect& bounds, bool num, bool anim);


#endif // __EXAMPLEEDITOR_H__