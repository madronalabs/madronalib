
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLSignalView.h"

const char* MLSignalView::kViewProcName = "signal_viewer_proc";

MLSignalView::MLSignalView(MLWidget* w, ml::Symbol attr, int size, int priority) :
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

void MLSignalView::setupSignalView (MLDSPEngine* pEng, const ml::Symbol sigName, int voices)
{
	mViewingSignal = true;
	mpEngine = pEng;
	mSignalName = sigName;
	if(voices != mVoices)
	{
		mpWidget->setProperty(ml::Symbol("voices"), voices);
		mVoices = voices;
	}
}

void MLSignalView::sendSignalToWidget(const MLSignal& signal, int samples, int voices)
{		
	const int viewSamples = ml::min(mSize, samples);
	mpWidget->viewSignal(mAttr, signal, viewSamples, voices);
}
