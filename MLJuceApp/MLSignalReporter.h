
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef __ML_SIGNAL_REPORTER_H
#define __ML_SIGNAL_REPORTER_H

//#include "JuceHeader.h"

//#include "MLModel.h"
//#include "MLWidget.h"
//#include <map>

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
	
	void addSignalViewToMap(MLSymbol p, MLWidget* w, MLSymbol attr, int size);
	void viewSignals();

protected:
	MLPluginProcessor* mpProcessor;
	MLSignalViewListMap mSignalViewsMap;
	
};


#endif // __ML_SIGNAL_REPORTER_H