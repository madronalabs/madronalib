
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLSignalReporter.h"

MLSignalReporter::MLSignalReporter(MLPluginProcessor* p) :
	mpProcessor(p),
    mViewIndex(0)
{
}

MLSignalReporter::~MLSignalReporter()
{

}

// add another signal view to our map, to be serviced periodically.
//
void MLSignalReporter::addSignalViewToMap(MLSymbol alias, MLWidget* w, MLSymbol attr, int viewSize, int priority)
{
 	MLDSPEngine* const pEngine = mpProcessor->getEngine();
	if(!pEngine) return;	

	// first, find published signal if available and add read buffers. 
	int bufSize = pEngine->getPublishedSignalBufferSize(alias);
	if(bufSize > 0)
	{
		// if signal buffer does not already exist, add one
		MLSymbolToSignalMap::const_iterator it = mSignalBuffers.find(alias);
		if (it == mSignalBuffers.end()) 
		{
			// add buffers so we can see if the signal has been changed since the last view
			mSignalBuffers[alias] = MLSignalPtr(new MLSignal(bufSize));
			mSignalBuffers2[alias] = MLSignalPtr(new MLSignal(bufSize));
			mSignalBuffers2[alias]->fill(-1.f); // force initial view of zero signal
		}
		
		viewSize = min(viewSize, bufSize);

        // add the list of widgets and attributes for viewing
        mSignalViewsMap[alias].push_back(MLSignalViewPtr(new MLSignalView(w, attr, viewSize, priority)));
        
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
		MLError() << "MLSignalReporter::addSignalViewToMap: no published signal " << alias << "!\n";
	}
}

// for one signal in the map, run all views in the view list matching priority.
//
int MLSignalReporter::viewOneSignal(MLSymbol signalName, int priority)
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
    int samples = pEngine->readPublishedSignal(signalName, buffer1);
    
    if(samples > 0)
    {
        // test the RMS change of the signal buffer to see if a redraw is needed.
        // TODO better metric taking display range of signal viewer component into account.
        // all this does currently is prevents some of the tiny changes from parameter
        // drifts from causing redraws. 
        float changeSum = buffer1.rmsDiff(buffer2);
        buffer2 = buffer1;
        if(changeSum > 0.001f)
        {
            // send signal to each signal view in its viewer list.
            MLSignalViewList viewList = mSignalViewsMap[signalName];
            
            for(MLSignalViewList::iterator it2 = viewList.begin(); it2 != viewList.end(); it2++)
            {
                // send engine and signal information to viewer proc.
                MLSignalViewPtr pV = *it2;
                if(pV->mPriority >= priority)
                {
                    // should not be needed every time, only when # of voices changes
                    pV->setupSignalView(pEngine, signalName, voices);
                    
                    pV->sendSignalToWidget(buffer1, samples);
                    drawn++;
                }
           }
        }
    }
    
    return drawn;
}

void MLSignalReporter::viewSignals()
{
	if(!mpProcessor) return;	
 	MLDSPEngine* const pEngine = mpProcessor->getEngine();
	if(!pEngine) return;
    
    const int maxSignalsToDraw = 4;
	MLSymbol signalName;
    
    // first service all views that have high priority set
    for(ViewPriorityMap::iterator it = mViewPriorityMap.begin();
        it != mViewPriorityMap.end(); it++ )
    {
        signalName = it->first;
        int p = it->second;
        if(p > 0)
        {
            viewOneSignal(signalName, p);
        }
    }

    // wrap through all signals to see if they need servicing,
    // bailing out if maxSignalsToDraw are serviced    
    int nSignals = mSignalNames.size();
    int signalsDrawn = 0;
    for(int i = 0; (i<nSignals) && (signalsDrawn < maxSignalsToDraw); ++i)
    {
        mViewIndex++;
        if(mViewIndex >= nSignals)
        {
            mViewIndex = 0;
        }
        signalName = mSignalNames[mViewIndex];
        signalsDrawn += viewOneSignal(signalName);
    }
}


