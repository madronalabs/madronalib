
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef __EXAMPLECONTROLLER_H__
#define __EXAMPLECONTROLLER_H__

#include "AppConfig.h"

#include "MLTime.h"

#include "MLUIBinaryData.h"
#include "MLPluginController.h"

#include "MLUIBinaryData.h"

#include "MLResponder.h"
#include "MLReporter.h"

#include "pa_ringbuffer.h"
#include "OscReceivedElements.h"
#include "OscPacketListener.h"

#include "MLProcInputToSignals.h"
#include "MLInputProtocols.h"

class MLInstrumentController  : 
	public MLPluginController,
	public Timer
{
public:
    MLInstrumentController(MLPluginProcessor* const ownerProcessor);
    ~MLInstrumentController();
	void initialize();
	void timerCallback();
	void doInfrequentTasks();

	// from MLButton::Listener
	void buttonClicked (MLButton*);

	void adaptUIToPatch();
	
private:	
    // (MLInstrumentController copy constructor and operator= being generated..)
    MLInstrumentController (const MLInstrumentController&);
    const MLInstrumentController& operator= (const MLInstrumentController&);
	
	MLPluginProcessor* const mpProcessor;
};



#endif // __EXAMPLECONTROLLER_H__