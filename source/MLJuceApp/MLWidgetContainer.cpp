
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLWidgetContainer.h"

MLWidgetContainer::MLWidgetContainer(MLWidget* pRoot) : mpRootWidget(pRoot)
{
}

MLWidgetContainer::~MLWidgetContainer()
{
}

// add a widget to both the ML and Juce worlds.  The widget is retained by the Juce Component.
// 
void MLWidgetContainer::addWidget(MLWidget* pW, const ml::Symbol name)
{	
	ml::Symbol newName;
	if (name)
	{
		if(mWidgets.find(name) != mWidgets.end())
		{
			//debug() << "MLWidgetContainer::addWidget: name " << name << " already taken! \n";			
			//debug() << mWidgets.size() << " widgets:\n";
			dumpWidgets();
			ml::theSymbolTable().dump();
			ml::theSymbolTable().audit();
		}
		else
		{
			newName = name;
		}
	}
	else // get name for anon widgets
	{
		newName = mWidgetNamer.nextName();
	}

	if(newName)
	{
		mWidgets[newName] = pW;
		pW->setWidgetName(newName);
	}
    
	pW->setContainer(this);
	
	// give root context pointer to all new containers
	if(pW->isWidgetContainer())
	{
		MLWidgetContainer& resultContainer = static_cast<MLWidgetContainer&>(*pW);
		
		// all Containers in a tree of Widgets have pointers to the same root context. 
		resultContainer.setRootView(getRootView());
	}
	
    // add parent JUCE component 
    getComponent()->addChildComponent(pW->getComponent());
}

MLWidget* MLWidgetContainer::getWidget(ml::Symbol name)
{
	MLWidget* result = nullptr;
	std::map<ml::Symbol, MLWidget*>::iterator look = mWidgets.find(name);
	
	if(look != mWidgets.end())
	{
		result = (look->second);
	}
	else // look in containers
	{
		std::map<ml::Symbol, MLWidget*>::iterator it;
		for(it = mWidgets.begin(); it != mWidgets.end(); ++it)
		{
			MLWidget* subWidget = (it->second);
			if(subWidget->isWidgetContainer())
			{		
				if((result = subWidget->getWidget(name))) 
				{
					break;
				}
			}
		}
	}
	return result;
}

void MLWidgetContainer::renameWidget(MLWidget* pW, const ml::Symbol newName)
{
	bool found = false;
	std::map<ml::Symbol, MLWidget*>::iterator it;
	for(it = mWidgets.begin(); it != mWidgets.end(); ++it)
	{
		MLWidget* pB = (it->second);
		if(pB == pW)
		{
			mWidgets.erase(it);
			addWidget(pW, newName);
			found = true;
			break;
		}
	}
	
	if (!found)
	{
		//debug() << " MLWidgetContainer::renameWidget: widget not found!\n";
	}
}


void MLWidgetContainer::dumpWidgets(int depth)
{
	std::map<ml::Symbol, MLWidget*>::iterator it;
	
	std::string spaceStr;
	for(int i=0; i<depth; ++i)
	{
		spaceStr += "  ";
	}
	
	for(it = mWidgets.begin(); it != mWidgets.end(); it++)
	{
		//ml::Symbol name = it->first;
		MLWidget* widget = it->second;
		
		//debug() << spaceStr	<< name << "\n";
		if (widget->isWidgetContainer())
		{
			//debug() << spaceStr << "contains:\n";
			MLWidgetContainer* pC = static_cast<MLWidgetContainer*>(widget);
			pC->dumpWidgets(depth + 1);
		}
	}
}
