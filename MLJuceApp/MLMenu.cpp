
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLMenu.h"

MLMenu::MLMenu() :
	mNumItems(0)
{
}

MLMenu::~MLMenu()
{
}

void MLMenu::addItem(const std::string& pItemStr)
{
	mNumItems++;
	mJuceMenu.addItem(mNumItems, pItemStr.c_str());
	mItems.push_back(pItemStr);
}

void MLMenu::addItems(std::list<std::string>& pItemList)
{
	std::list<std::string>::iterator it;
	for(it = pItemList.begin(); it != pItemList.end(); it++)
	{
		addItem(*it);
	}
}

void MLMenu::clear()
{
	mNumItems = 0;
	mJuceMenu.clear();
	mItems.clear();
}

const std::string& MLMenu::getItemString(int idx)
{
	return mItems[idx];
}

PopupMenu& MLMenu::getJuceMenu()
{
	return mJuceMenu;
}

