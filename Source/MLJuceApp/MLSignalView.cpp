
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLSignalView.h"

const char* MLSignalView::kViewProcName = "signal_viewer_proc";

MLSignalView::MLSignalView(MLWidget* w, MLSymbol attr, int size, int priority) :
	mViewingSignal(false),
	mpEngine(0),
	mpWidget(w),
	mAttr(attr),
    mSize(size),
    mPriority(priority),
	mNumSignals(1)
{

}

MLSignalView::~MLSignalView()
{
}

void MLSignalView::setupSignalView (MLDSPEngine* pEng, const MLSymbol sigName, unsigned voices)
{
	mViewingSignal = true;
	mpEngine = pEng;
	mSignalName = sigName;
	mpWidget->setAttribute(MLSymbol("voices"), voices);
}

void MLSignalView::sendSignalToWidget(const MLSignal& signal, int samples)	
{		
	const int viewSamples = min(mSize, samples);
	mpWidget->viewSignal(mAttr, signal, viewSamples);
}