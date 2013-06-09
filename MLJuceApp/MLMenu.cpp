
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLMenu.h"

MLMenu::MLMenu(const char* name) :
	mName(name),
	mNumItems(0),
	mItemOffset(0)
{
	mNullStr = "<null>";
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

void MLMenu::addSubMenu(MLMenuPtr m, bool enabled)
{	
	mJuceMenu.addSubMenu(m->getName().c_str(), m->getJuceMenu(), enabled);
	std::vector<std::string>::const_iterator it;
	const std::vector<std::string> v = m->getItemVector();	
	for(it = v.begin(); it != v.end(); it++)
	{
		mItems.push_back(*it);
		mNumItems++;
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
	return mNullStr;
}

PopupMenu& MLMenu::getJuceMenu()
{
	return mJuceMenu;
}

