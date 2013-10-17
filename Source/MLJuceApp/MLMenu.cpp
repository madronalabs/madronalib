
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLMenu.h"

static const std::string kNullStr("<null>");

MLMenu::MLMenu() :
    mName(""),
    mItemOffset(0),
	mNumItems(0)
{
}

MLMenu::MLMenu(const MLSymbol name) :
    mName(name),
    mItemOffset(0),
    mNumItems(0)
{
}

MLMenu::~MLMenu()
{
}

void MLMenu::addItem(const char* name, bool enabled)
{	
	mJuceMenu.addItem(mNumItems + mItemOffset + 1, name, enabled);
	mNumItems++;
	mItems.push_back(std::string(name));
}

void MLMenu::addItem(const std::string& name, bool enabled)
{
	addItem(name.c_str(), enabled);
}

void MLMenu::addItems(const std::vector<std::string>& items)
{
	std::vector<std::string>::const_iterator it;
	for(it = items.begin(); it != items.end(); it++)
	{
		addItem(it->c_str(), true);
	}
}

void MLMenu::addSubMenu(MLMenuPtr m, const char* name, bool enabled)
{
	mJuceMenu.addSubMenu(name, m->getJuceMenu(), enabled);
	std::vector<std::string>::const_iterator it;
	const std::vector<std::string> v = m->getItemVector();
    
    // add each element of submenu in turn to our flat item vector
	for(it = v.begin(); it != v.end(); it++)
	{
		mItems.push_back(*it);
		mNumItems++;
	}
	return;
}

void MLMenu::appendMenu(MLMenuPtr m)
{
	std::vector<std::string>::const_iterator it;
	const std::vector<std::string> v = m->getItemVector();
    
    // TODO recursive add
    
    // add each element of other menu in turn to our flat item vector
	for(it = v.begin(); it != v.end(); it++)
	{
        debug() << "item " << *it << "\n";
		addItem((const std::string&)*it);
	}
	return;
}

void MLMenu::addSeparator()
{
	mJuceMenu.addSeparator();
}

void MLMenu::clear()
{
	mJuceMenu.clear();
	mNumItems = 0;
	mItems.clear();
}

const std::string& MLMenu::getItemString(int idx)
{
	if(within(idx, 0, mNumItems))
	{
		return mItems[idx];
	}
	return kNullStr;
}

PopupMenu& MLMenu::getJuceMenu()
{
	return mJuceMenu;
}

void MLMenu::dump()
{
 	std::vector<std::string>::const_iterator it;
    
    debug() << "menu " << mName << "\n";
    
    // add each element of submenu in turn to our flat item vector
    int idx = 0;
	for(it = mItems.begin(); it != mItems.end(); it++)
	{
		debug() << "    " << idx++ << *it << "\n";
	}
   
}

