
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLSignalReporter.h"

MLSignalReporter::MLSignalReporter(MLPluginProcessor* p) :
	mpProcessor(p),
    mViewIndex(0),
	mNeedsRedraw(true)
{
	
}

MLSignalReporter::~MLSignalReporter()
{

}

// add another signal view to our map, to be serviced periodically.
//
MLSignalView* MLSignalReporter::addSignalViewToMap(MLSymbol alias, MLWidget* w, MLSymbol attr, int viewSize, int priority)
{
 	MLDSPEngine* const pEngine = mpProcessor->getEngine();
	if(!pEngine) return nullptr;	
	MLSignalView* pNewView = nullptr;
	
	// first, find published signal if available and add read buffers. 
	int bufSize = pEngine->getPublishedSignalBufferSize(alias);
    int voices = pEngine->getPublishedSignalVoices(alias);
	if(bufSize > 0)
	{
		// if signal buffer does not already exist, add one.
        // Allocates a signal with one row per voice. 
		MLSymbolToSignalMap::const_iterator it = mSignalBuffers.find(alias);
		if (it == mSignalBuffers.end()) 
		{
			// add buffers so we can see if the signal has been changed since the last view
			mSignalBuffers[alias] = MLSignalPtr(new MLSignal(bufSize, voices));
			mSignalBuffers2[alias] = MLSignalPtr(new MLSignal(bufSize, voices));
			mSignalBuffers2[alias]->fill(-1.f); // force initial view of zero signal
		}
		
		viewSize = min(viewSize, bufSize);

        // add the list of widgets and attributes for viewing
		pNewView = new MLSignalView(w, attr, viewSize, priority);
        mSignalViewsMap[alias].push_back(MLSignalViewPtr(pNewView));
        
        // push name to name vector
        mSignalNames.push_back(alias);
        
        // mark priority map if high priority
        if(priority > 0)
        {
            int p = mViewPriorityMap[alias]; // creates and returns 0 if not found
            mViewPriorityMap[alias] = max(p, priority);
        }
	}
	else
	{
		debug() << "MLSignalReporter::addSignalViewToMap: no published signal " << alias << "!\n";
	}
	return pNewView;
}

bool signalsAreDifferent(const MLSignal& a, const MLSignal& b, int samplesToCompare, int voices)
{
    bool result = false;
    float totalDiff = 0.0f;
    for(int j = 0; j < voices; ++j)
    {
        for(int i = 0; i<samplesToCompare; ++i)
        {
            float fa = a(i, j);
            float fb = b(i, j);
            float df = fa - fb;
            totalDiff += df*df;
            if(totalDiff > 0.001f)
            {
                result = true;
				goto bail;
            }
        }
    }
    
bail:
    return result;
}

// for one signal in the map, run all views in the view list matching priority.
//
int MLSignalReporter::viewOneSignal(MLSymbol signalName, bool forceView, int priority)
{
 	MLDSPEngine* const pEngine = mpProcessor->getEngine();
	if(!pEngine) return 0;
 
    // get temp buffers
    MLSignal& buffer1 = *(mSignalBuffers[signalName].get());
    MLSignal& buffer2 = *(mSignalBuffers2[signalName].get());
    
    // TODO revisit this-- we should not need to count the signals each time.
    // only needed when we recompile or turn voices on / off.
    int voices = mpProcessor->countSignals(signalName);
    
    // read signal into buffer and check for change.
    // TODO post ring buffer, we have to look at all the samples in the buffer
    // to detect changes. Instead, the DSP engine can keep track of what
    // published signals have changed since the last read, and we can ask it.
    int drawn = 0;
    int samplesRead = pEngine->readPublishedSignal(signalName, buffer1);
    int samplesRequested = buffer1.getWidth(); // samples asked for by readPublishedSignal()
    
    // if the buffer was full, compare to see if a redraw is needed. 
    if(samplesRead == samplesRequested) 
    {
		bool changed = signalsAreDifferent(buffer1, buffer2, samplesRead, voices);
        if(changed || forceView)
        {
			// send signal to each signal view in its viewer list.
            MLSignalViewList viewList = mSignalViewsMap[signalName];
            for(MLSignalViewList::iterator it2 = viewList.begin(); it2 != viewList.end(); it2++)
            {
                // send engine and signal information to viewer proc.  
                MLSignalViewPtr pV = *it2;
                if(pV->mPriority >= priority)
                {
                    // again TODO should not be needed every time, only when # of voices changes
                    pV->setupSignalView(pEngine, signalName, voices);                    
                    pV->sendSignalToWidget(buffer1, samplesRead, voices);
                    drawn++;
                }
            }
			buffer2 = buffer1;
        }
    }
    return drawn;
}

void MLSignalReporter::viewSignals()
{
	if(mNeedsRedraw)
	{
		mNeedsRedraw = false;
		viewAllSignals();
	}
	else
	{
		viewChangedSignals();
	}
}

void MLSignalReporter::redrawSignals()
{
	mNeedsRedraw = true;
}

void MLSignalReporter::viewChangedSignals()
{
	if(!mpProcessor) return;
 	MLDSPEngine* const pEngine = mpProcessor->getEngine();
	if(!pEngine) return;
    
	const int maxSignalsToViewPerFrame = 6; 
	MLSymbol signalName;
	
	// first service all views that have high priority set
    for(ViewPriorityMap::iterator it = mViewPriorityMap.begin();
        it != mViewPriorityMap.end(); it++ )
    {
        signalName = it->first;
        int p = it->second;
        if(p > 0)
        {
            viewOneSignal(signalName, false, p);
        }
    }
    
    // wrap through all signals to see if they need servicing,
    // bailing out if maxSignalsToDraw are serviced
    int nSignals = mSignalNames.size();
    int signalsDrawn = 0;
    for(int i = 0; (i<nSignals) && (signalsDrawn < maxSignalsToViewPerFrame); ++i)
    {
        mViewIndex++;
        if(mViewIndex >= nSignals)
        {
            mViewIndex = 0;
        }
        signalName = mSignalNames[mViewIndex];
        int p = mViewPriorityMap[signalName];
        if(p == 0)
        {
            signalsDrawn += viewOneSignal(signalName, false);
        }
    }
}

void MLSignalReporter::viewAllSignals()
{
	if(!mpProcessor) return;
 	MLDSPEngine* const pEngine = mpProcessor->getEngine();
	if(!pEngine) return;
    
    int nSignals = mSignalNames.size();
    for(int i = 0; i<nSignals; ++i)
    {
        const bool forceView = true;
        const MLSymbol signalName = mSignalNames[i];		
        viewOneSignal(signalName, forceView);
    }
}


