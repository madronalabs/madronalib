
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLMenu.h"

MLMenu::MLMenu(const char* name) :
	mName(name),
	mItemOffset(0)
{
}

MLMenu::~MLMenu()
{
}

void MLMenu::addItem(const char* name, bool enabled)
{
	int size = mItems.size();
//debug() << "menu adding item " << name << "\n";	
	mJuceMenu.addItem(size + mItemOffset + 1, name, enabled);
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
		mItems.push_back(*it);
	}
}

void MLMenu::addSubMenu(MLMenuPtr m, bool enabled)
{	
	mJuceMenu.addSubMenu(m->getName().c_str(), m->getJuceMenu(), enabled);

	addItems(m->getItemVector());
	
	return;
}

void MLMenu::addSeparator()
{
	mJuceMenu.addSeparator();
}

void MLMenu::clear()
{
	mJuceMenu.clear();
	mItems.clear();
}

const std::string& MLMenu::getItemString(int idx)
{
debug() << "item " << idx << ":" << mItems[idx] << "\n";
	return mItems[idx];
}

PopupMenu& MLMenu::getJuceMenu()
{
	return mJuceMenu;
}

