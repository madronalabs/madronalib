
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef __ML_SIGNALVIEWER_H__
#define __ML_SIGNALVIEWER_H__

#include "MLWidget.h"
#include "MLProcContainer.h"
#include "MLSymbol.h"
//#include "MLPath.h"
#include "MLSignal.h"
#include "MLDSPEngine.h"

class MLSignalView
{
friend class MLSignalReporter;
public:
	static const char* kViewProcName;

	MLSignalView(MLWidget* w, ml::Symbol attr, int size = kMLSignalViewBufferSize, int priority = 0);
	~MLSignalView();
	void setupSignalView (MLDSPEngine* pEng, const ml::Symbol sigName, int voices);
	void sendSignalToWidget(const MLSignal& signal, int samples, int voices);	
	const MLRect& getRepaintRect() const;	
	ml::Symbol getSignalName() { return mSignalName; }	

private:
	int mVoices;
	bool mViewingSignal;
	MLDSPEngine* mpEngine;	
	ml::Symbol mSignalName;
	MLWidget* mpWidget;
	ml::Symbol mAttr;
	int mSize;
    int mPriority;
};


#endif