
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLSignalView.h"

const char* MLSignalView::kViewProcName = "signal_viewer_proc";

MLSignalView::MLSignalView(MLWidget* w, MLSymbol attr, int size, int priority) :
	mVoices(0),
	mViewingSignal(false),
	mpEngine(0),
	mpWidget(w),
	mAttr(attr),
	mSize(size),
	mPriority(priority)
{
}

MLSignalView::~MLSignalView()
{
}

void MLSignalView::setupSignalView (MLDSPEngine* pEng, const MLSymbol sigName, int voices)
{
	mViewingSignal = true;
	mpEngine = pEng;
	mSignalName = sigName;
	if(voices != mVoices)
	{
		mpWidget->setProperty(MLSymbol("voices"), voices);
		mVoices = voices;
	}
}

void MLSignalView::sendSignalToWidget(const MLSignal& signal, int samples, int voices)
{		
	const int viewSamples = min(mSize, samples);
	mpWidget->viewSignal(mAttr, signal, viewSamples, voices);
}
