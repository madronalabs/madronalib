
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef __ML_SIGNAL_REPORTER_H
#define __ML_SIGNAL_REPORTER_H

// MLSignalReporter is a mixin class used by objects like MLPluginController
// that display one or more published signals from the DSP engine. 
// 

#include "MLPluginProcessor.h"
#include "MLSignalViewer.h"

// --------------------------------------------------------------------------------
#pragma mark signal viewing 

typedef std::tr1::shared_ptr<MLSignalViewer> MLSignalViewPtr;
typedef std::list<MLSignalViewPtr> MLSignalViewList;
typedef std::map<MLSymbol, MLSignalViewList> MLSignalViewListMap;

// --------------------------------------------------------------------------------
#pragma mark MLSignalReporter 

class MLSignalReporter 
{
public:
	MLSignalReporter(MLPluginProcessor* p);
    ~MLSignalReporter();
	
	// add a signal view entry to the map and connect it to a new signal viewer.
	void addSignalViewToMap(MLSymbol p, MLWidget* w, MLSymbol attr, int size);
	
	// view all of the signals in the map.
	void viewSignals();

protected:
	MLPluginProcessor* mpProcessor;
	MLSignalViewListMap mSignalViewsMap;
	
};

#endif // __ML_SIGNAL_REPORTER_H