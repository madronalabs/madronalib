
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLSignalViewer.h"

const char* MLSignalViewer::kViewProcName = "signal_viewer_proc";

MLSignalViewer::MLSignalViewer(MLWidget* w, MLSymbol attr, int size) :
	mViewingSignal(false),
	mpEngine(0),
	mpWidget(w),
	mAttr(attr),
	mSize(size)
{
	mViewBuffer.setDims(size);
	mViewBuffer.fill(-1.f); // force initial view
	mViewBuffer2.setDims(size);
	mViewBuffer2.clear();
}

MLSignalViewer::~MLSignalViewer()
{
}

void MLSignalViewer::setupViewSignal (MLDSPEngine* pEng, const MLSymbol sigName, unsigned voices)
{
	mViewingSignal = true;
	mpEngine = pEng;
	mSignalName = sigName;
	mpWidget->setAttribute(MLSymbol("voices"), voices);
}

void MLSignalViewer::doViewSignal()	
{		
	if (!mpEngine) return;
	
	// copy previous buffer
	mViewBuffer2 = mViewBuffer;	
	int samples = mpEngine->readPublishedSignal(mSignalName, mViewBuffer);	

	// if identical to previous signal, bail.
	if (mViewBuffer == mViewBuffer2) return;
	
	if ((samples > 0) && (samples <= mSize))
	{
		mpWidget->viewSignal(mAttr, mViewBuffer, samples);
	}
	else
	{
		// Bad reads may happen while changing the number of voices, or any time ring
		// buffers are empty. 
		// debug() << "doViewSignal() bad read of signal 	" << mSignalName << "!\n";
	}
}

