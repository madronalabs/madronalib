
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLMenu.h"

static const std::string kNullStr("<null>");
static const std::string kSeparatorStr("---");

#pragma mark MLMenu::Node

MLMenu::Node::Node() :
	mItemNumber(0),
	mEnabled(false),
	mTicked(false)
{
	
}

MLMenu::Node::~Node()
{
	
}

void MLMenu::Node::clear()
{
    map.clear();
    index.clear();
    subMenu = JuceMenuPtr();
}

bool MLMenu::Node::isValid()
{
	return mItemNumber > 0;
}

void MLMenu::Node::dump(int level)
{
    std::list<std::string>::const_iterator it;
	for(it = index.begin(); it != index.end(); it++)
	{
        const std::string& name = *it;
        StringToMenuNodeMapT::const_iterator it2 = map.find(name);
        NodePtr node = it2->second;
        for(int i=0; i<level*4; ++i)
        {
            debug() << " ";
        }
		debug() << name << " #" << node->mItemNumber << ", (" << node->index.size() << ")\n";
        node->dump(level + 1);
	}
    debug() << "\n";
}

MLMenu::NodePtr MLMenu::Node::getSubnodeByName(const std::string& name)
{
	return map.find(name)->second;
}

int MLMenu::Node::renumberItems(int n)
{
    bool isLeaf = (index.size() == 0);
    if(isLeaf)
    {
        mItemNumber = n++;
    }
    else
    {
        std::list<std::string>::const_iterator it;
        for(it = index.begin(); it != index.end(); it++)
        {
            const std::string& name = *it;
            if(name != kSeparatorStr)
            {
                StringToMenuNodeMapT::const_iterator it2 = map.find(name);
                NodePtr node = it2->second;
                n = node->renumberItems(n);
            }
        }
    }
    return n;
}

int MLMenu::Node::getNodeSize(int n)
{
    bool isLeaf = (index.size() == 0);
    if(isLeaf)
    {
        n++;
    }
    else
    {
        StringToMenuNodeMapT::const_iterator it;
        for(it = map.begin(); it != map.end(); it++)
        {
            std::string name = it->first;
            if(name != kSeparatorStr)
            {
                NodePtr node = it->second;
                n = node->getNodeSize(n);
            }
        }
    }
    return n;
}

void MLMenu::Node::buildFullNameIndex(std::vector<std::string>& nameVec, const std::string& path)
{
    bool isLeaf = (index.size() == 0);
    if(isLeaf)
    {
        int size = nameVec.size();
        if(mItemNumber >= size)
        {
            nameVec.resize(mItemNumber + 1);
            for(int i = size; i <= mItemNumber; ++i)
            {
                nameVec[i] = std::string();
            }
        }
        nameVec[mItemNumber] = path;
    }
    else
    {
        std::list<std::string>::const_iterator it;
        for(it = index.begin(); it != index.end(); it++)
        {
            const std::string& name = *it;
            StringToMenuNodeMapT::const_iterator it2 = map.find(name);
            const std::string& nodeName = it2->first;
            NodePtr node = it2->second;
            std::string fullPath;
            if(path == std::string(""))
            {
                fullPath = nodeName;
            }
            else
            {
                fullPath = path + "/" + nodeName;
            }
            node->buildFullNameIndex(nameVec, fullPath);
        }
    }
}

void MLMenu::Node::addToJuceMenu(const std::string& name, JuceMenuPtr pMenu, bool root)
{
    bool isLeaf = (index.size() == 0);
    if(isLeaf)
    {
        if(name != kSeparatorStr)
        {
            pMenu->addItem(mItemNumber, mDisplayPrefix + ml::stringUtils::stripExtension(ml::stringUtils::getShortName(name)), mEnabled, mTicked);
        }
        else
        {
            pMenu->addSeparator();
        }
    }
    else if (root)
    {
        // for root, add children directly to menu
        // iterate through children and add to submenu
        std::list<std::string>::const_iterator it;
        for(it = index.begin(); it != index.end(); it++)
        {
            const std::string& name = *it;
            StringToMenuNodeMapT::const_iterator it2 = map.find(name);
            const std::string& nodeName = it2->first;
            NodePtr node = it2->second;
            node->addToJuceMenu(nodeName, pMenu, false);
        }        
    }
    else
    {
        // a normal node with children. 
        // make persistent submenu for this node
        subMenu = JuceMenuPtr(new juce::PopupMenu);
        
        // iterate through children and add to submenu
        std::list<std::string>::const_iterator it;
        for(it = index.begin(); it != index.end(); it++)
        {
            const std::string& name = *it;
            StringToMenuNodeMapT::const_iterator it2 = map.find(name);
            const std::string& nodeName = it2->first;
            NodePtr node = it2->second;
            node->addToJuceMenu(nodeName, subMenu, false);
        }
        
        // add submenu to parent
        pMenu->addSubMenu(name, *subMenu);
    }
}

#pragma mark MLMenu

MLMenu::MLMenu() :
    mName(""),
    mRoot(new Node()),
    mHasIndex(false)
{
}

MLMenu::MLMenu(const MLSymbol name) :
    mName(name),
    mRoot(new Node()),
    mHasIndex(false)
{
}

MLMenu::~MLMenu()
{
}

void MLMenu::addItem(const std::string& name, bool enabled, bool ticked)
{
    NodePtr n(new Node());
    n->mEnabled = enabled;
    n->mTicked = ticked;
    mRoot->map[name] = n;
    mRoot->index.push_back(name);
}

MLMenu::NodePtr MLMenu::getItem(const std::string& name)
{
    NodePtr n;
    StringToMenuNodeMapT::const_iterator it = mRoot->map.find(name);
    if(it != mRoot->map.end())
    {
        n = it->second;
    }
    return n;
}

void MLMenu::addItems(const std::vector<std::string>& items)
{
	std::vector<std::string>::const_iterator it;
	for(it = items.begin(); it != items.end(); it++)
	{
		addItem(*it, true);
	}
}

void MLMenu::addSubMenu(MLMenuPtr m)
{
	// copy NodePtr into our node map
	const std::string& subMenuName = m->getName().getString();
	mRoot->map[subMenuName] = m->mRoot;
	mRoot->index.push_back(subMenuName);
}

void MLMenu::addSubMenu(MLMenuPtr m, const std::string& name)
{
	// copy NodePtr into our node map
	mRoot->map[name] = m->mRoot;
	mRoot->index.push_back(name);
}

// append all items in root to the menu m
void MLMenu::appendMenu(MLMenuPtr m)
{
    // add each element of other menu in turn to our item map
    StringToMenuNodeMapT::const_iterator it;
	for(it = m->mRoot->map.begin(); it != m->mRoot->map.end(); it++)
	{
        const std::string& name = it->first;
        NodePtr node = it->second;
        mRoot->map[name] = node;
        mRoot->index.push_back(name);
	}
}

void MLMenu::buildIndex()
{
    mRoot->renumberItems();
    std::string startPath("");
    mRoot->buildFullNameIndex(mFullNamesByIndex, startPath);
    mHasIndex = true;
}
    
void MLMenu::addSeparator()
{
    addItem(kSeparatorStr);
}

void MLMenu::clear()
{
	mRoot->clear();
    mFullNamesByIndex.clear();
}

// TODO use resource map
const std::string MLMenu::getMenuItemPath(int idx)
{
    if(!mHasIndex)
    {
        buildIndex();
    }
    int items = mFullNamesByIndex.size();
    // items are 1-indexed
	if(within(idx, 1, items + 1))
	{
        return mFullNamesByIndex[idx];
	}
	return kNullStr;
}

JuceMenuPtr MLMenu::getJuceMenu()
{
    buildIndex();
    JuceMenuPtr jm(new PopupMenu());    
    mRoot->addToJuceMenu(mName.getString(), jm);
    return jm;
}

void MLMenu::dump()
{
    debug() << " dump of menu " << mName << ":\n";
    mRoot->dump();
    debug() << "\n";
}

