
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

// add another signal view to our map, to be serviced periodically.
//
void MLSignalReporter::addSignalViewToMap(MLSymbol p, MLWidget* w, MLSymbol attr, int size)
{
	// add a buffer so we can see if the signal has been changed since the last view
	mSignalBuffers[p] = MLSignalPtr(new MLSignal(size));
	mSignalBuffers2[p] = MLSignalPtr(new MLSignal(size));
	mSignalBuffers2[p]->fill(-1.f); // force initial view of zero signal

	// add the list of widgets and attributes for viewing
	mSignalViewsMap[p].push_back(MLSignalViewPtr(new MLSignalView(w, attr, size)));
}

void MLSignalReporter::viewSignals() 
{	
	if(!mpProcessor) return;	
 	MLDSPEngine* const pEngine = mpProcessor->getEngine();
	if(!pEngine) return;	
	
	// for each named signal in map
	MLSymbol signalName;
 	MLSignalViewListMap::iterator it;		
	for(it = mSignalViewsMap.begin(); it != mSignalViewsMap.end(); it++)
	{
		signalName = it->first;		
		
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
		
		buffer2 = buffer1;	
		int samples = pEngine->readPublishedSignal(signalName, buffer1);	
		if(samples > 0)
		{
			const bool changed = (buffer1 != buffer2);

			if(changed)
			{
				// send signal to each signal view in list.
				MLSignalViewList viewList = it->second;
				MLSignalViewList::iterator it2;		

	/*
				int s = viewList.size();
				if(s > 1)
				{
					debug() << "viewing " << signalName	<< " : " << s << "views\n";
				}
				int is = 0;
	*/												
																																			
				for(it2 = viewList.begin(); it2 != viewList.end(); it2++)
				{
					// send engine and signal information to viewer proc.
					MLSignalViewPtr pV = *it2; 
					
					// should not be needed every time, only when # of voices changes
					pV->setupSignalView(pEngine, signalName, voices);

	/*
					if(s > 1)
					{
						debug() << "    view " << ++is << " : attr " << pV->mAttr << 
							" of Widget " << pV->mpWidget->getWidgetName() << "\n";
					}
	*/				
					pV->sendSignalToWidget(buffer1, samples);
				}
			}
		}
	}
//	debug() << "MLSignalReporter::viewSignals() : " << changedSignals << " signals changed / viewed.\n";
}


