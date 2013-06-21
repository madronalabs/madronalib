
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef __ML_REPORTER_H
#define __ML_REPORTER_H

#include "MLModel.h"
#include "MLWidget.h"
#include <map>

// --------------------------------------------------------------------------------
#pragma mark param viewing 

class MLParamView
{
public:
	MLParamView(MLWidget* w, MLSymbol attr);
	~MLParamView();
	void view(const MLModelParam& v) const;
	
private:
	MLWidget* mpWidget;
	MLSymbol mAttr;
};

typedef std::tr1::shared_ptr<MLParamView> MLParamViewPtr;
typedef std::list<MLParamViewPtr> MLParamViewList;
typedef std::map<MLSymbol, MLParamViewList> MLParamViewListMap;

// --------------------------------------------------------------------------------
#pragma mark MLReporter 

// Reporter watches a Model and reports its state. It doesn't know about Views, only Widgets.
// It reports values of parameters, using a map to a list of Widgets for each. 
// likewise, it can send Signals to signal display widgets. 
//
class MLReporter :
	public MLModelListener
{
public:
	MLReporter(MLModel* m);
    ~MLReporter();
	
	MLModel* getModel() { return mpModel; }

	// parameter viewing
	void addParamViewToMap(MLSymbol p, MLWidget* w, MLSymbol attr);
	void viewAllParams();
	void viewAllChangedParams();

	// MLModelListener interface
	void doParamChangeAction(MLSymbol param, const MLModelParam& oldVal, const MLModelParam& newVal);

protected:

	MLParamViewListMap mParamViewsMap;
};

#endif // __ML_REPORTER_H