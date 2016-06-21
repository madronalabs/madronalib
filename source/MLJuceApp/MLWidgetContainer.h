
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef __ML_WIDGET_CONTAINER_H__
#define __ML_WIDGET_CONTAINER_H__

#include "JuceHeader.h"
#include "MLWidget.h"

// maintains a View Component with grid dimensions set by parent. 
// provides handy UI component creation functions for grid.
// 
class MLWidgetContainer :
	public MLWidget
{
public:
	MLWidgetContainer();
    ~MLWidgetContainer();
	
	bool isWidgetContainer(void) { return true; }
	void addWidget(MLWidget* pW, const ml::Symbol name = ml::Symbol());
	MLWidget* getWidget(ml::Symbol name);
	void renameWidget(MLWidget* pW, const ml::Symbol newName);
	void dumpWidgets(int depth = 0);

protected:
	// currently Widgets are deleted by JUCE views, so don't retain here
	std::map<ml::Symbol, MLWidget*> mWidgets;
	
	MLNameMaker mWidgetNamer;
};

#endif // __ML_WIDGET_CONTAINER_H__
