
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLWidgetContainer.h"

MLWidgetContainer::MLWidgetContainer()
{
}

MLWidgetContainer::~MLWidgetContainer()
{
}

// add a widget to both the ML and Juce worlds.  The widget is retained by the Juce Component.
// 
void MLWidgetContainer::addWidget(MLWidget* pW, const MLSymbol name)
{	
	MLSymbol newName;
	if (name)
	{
		debug() << "\nlooking for widget " << name << " : " << name.getID() << "\n";

		if(mWidgets.find(name) != mWidgets.end())
		{
			debug() << "MLWidgetContainer::addWidget: name " << name << " already taken! \n";
			
			debug() << mWidgets.size() << " widgets:\n";
			for(auto w : mWidgets)
			{
				debug() << w.first << " : " << reinterpret_cast<unsigned long>(w.second) << "\n";
			}
//			dumpWidgets();
//			theSymbolTable().dump();
			
			theSymbolTable().audit();
		}
		else
		{
			// MLTEST
			debug() << "MLWidgetContainer::addWidget: adding widget " << name << "... \n";
			dumpWidgets();
			
			newName = name;
		}
	}
	else // get name for anon widgets
	{
		newName = mWidgetNamer.nextName();
	}

	if(newName)
	{
		std::string nameStr2 = newName.getString();

		mWidgets[nameStr2] = pW;
		pW->setWidgetName(newName);
	}
    
    // add parent JUCE component 
    getComponent()->addChildComponent(pW->getComponent());
}

MLWidget* MLWidgetContainer::getWidget(MLSymbol name)
{
	MLWidget* result = nullptr;
	std::map<MLSymbol, MLWidget*>::iterator look = mWidgets.find(name);
	
	if(look != mWidgets.end())
	{
		result = (look->second);
	}
	else // look in containers
	{
		std::map<MLSymbol, MLWidget*>::iterator it;
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

void MLWidgetContainer::renameWidget(MLWidget* pW, const MLSymbol newName)
{
	bool found = false;
	std::map<MLSymbol, MLWidget*>::iterator it;
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
		debug() << " MLWidgetContainer::renameWidget: widget not found!\n";
	}
}


void MLWidgetContainer::dumpWidgets(int depth)
{
	std::map<MLSymbol, MLWidget*>::iterator it;
	
	std::string spaceStr;
	for(int i=0; i<depth; ++i)
	{
		spaceStr += "  ";
	}
	
	for(it = mWidgets.begin(); it != mWidgets.end(); it++)
	{
		MLSymbol name = it->first;
		MLWidget* widget = it->second;
		
		debug() << spaceStr	<< name << "\n";
		if (widget->isWidgetContainer())
		{
			debug() << spaceStr << "contains:\n";
			MLWidgetContainer* pC = static_cast<MLWidgetContainer*>(widget);
			pC->dumpWidgets(depth + 1);
		}
	}
}
