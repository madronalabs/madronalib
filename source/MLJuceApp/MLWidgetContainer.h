
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef __ML_WIDGET_CONTAINER_H__
#define __ML_WIDGET_CONTAINER_H__

#include "JuceHeader.h"
#include "MLJuceApp/MLWidget.h"
#include "MLTextUtils.h"

class MLAppView;

// maintains a View Component with grid dimensions set by parent. 
// provides handy UI component creation functions for grid.
// 
class MLWidgetContainer :
	public MLWidget
{
public:
	MLWidgetContainer(MLWidget* pContainer, MLAppView* pRoot);
    ~MLWidgetContainer();
	
	bool isWidgetContainer(void) { return true; }
	void addWidget(MLWidget* pW, const ml::Symbol name = ml::Symbol());
	
	MLWidget* getWidget(ml::Symbol name);
	void renameWidget(MLWidget* pW, const ml::Symbol newName);
	void dumpWidgets(int depth = 0);
	
	// ----------------------------------------------------------------
	// root context pointer
	// TODO share the tree structure code with MLProc / getRootContext() !
	
	void setRootView(MLAppView* pC) { mpRootView = pC; }
	MLAppView* getRootView() const { return mpRootView; }

protected:

	MLAppView* mpRootView;
	
	// currently Widgets are deleted by JUCE views, so don't retain here
	std::map<ml::Symbol, MLWidget*> mWidgets;
	
	ml::textUtils::NameMaker mWidgetNamer;
};

#endif // __ML_WIDGET_CONTAINER_H__
