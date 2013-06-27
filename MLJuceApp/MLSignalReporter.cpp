
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLSignalReporter.h"

	
MLSignalReporter::MLSignalReporter(MLPluginProcessor* p) :
	mpProcessor(p)
{
}

MLSignalReporter::~MLSignalReporter()
{

}

void MLSignalReporter::addSignalViewToMap(MLSymbol p, MLWidget* w, MLSymbol attr, int size)
{
	mSignalViewsMap[p].push_back(MLSignalViewPtr(new MLSignalViewer(w, attr, size))); 
}

void MLSignalReporter::viewSignals() 
{	
	if(!mpProcessor) return;	
 	MLDSPEngine* const pEngine = mpProcessor->getEngine();
	if(!pEngine) return;	
	
	// for all signal views
	MLSymbol signalName;
 	MLSignalViewListMap::iterator it;	
	
	for(it = mSignalViewsMap.begin(); it != mSignalViewsMap.end(); it++)
	{
		signalName = it->first;		
		
		// TODO revisit this-- we should not need to count the signals each time.  
		// only needed when we recompile or turn voices on / off.
		int voices = mpProcessor->countSignals(signalName);

		// TODO see if the signal has changed since last view using
		// the constant signal flags.  Right now the individual samples are
		// checked in doViewSignal().
		bool changed = true;
		
		if(changed)
		{
			MLSignalViewList viewList = it->second;
			MLSignalViewList::iterator it2;
			for(it2 = viewList.begin(); it2 != viewList.end(); it2++)
			{
				// send engine and signal information to viewer proc.
				MLSignalViewPtr pV = *it2; 
				pV->setupViewSignal(pEngine, signalName, voices);
				
				// send signal data to the widget
				pV->doViewSignal();
			}
		}
	}
		
//	debug() << "MLSignalReporter::viewSignals() : " << changedSignals << " signals changed / viewed.\n";
}


