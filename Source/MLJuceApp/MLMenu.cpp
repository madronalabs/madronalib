
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLMenu.h"

static const std::string kNullStr("<null>");
static const std::string kSeparatorStr("---");

// --------------------------------------------------------------------------------
#pragma mark MLMenu::Node

void MLMenu::Node::dump(int level)
{
    std::list<std::string>::const_iterator it;
	for(it = index.begin(); it != index.end(); it++)
	{
        const std::string& name = *it;
        std::map<std::string, NodePtr>::const_iterator it2 = map.find(name);
        NodePtr node = it2->second;
        for(int i=0; i<level*4; ++i)
        {
            debug() << " ";
        }
		debug() << name << " #" << node->itemNumber << ", (" << node->index.size() << ")\n";
        node->dump(level + 1);
	}
    debug() << "\n";
}

int MLMenu::Node::renumberItems(int n)
{
    bool isLeaf = (index.size() == 0);
    if(isLeaf)
    {
        itemNumber = n++;
    }
    else
    {
        std::list<std::string>::const_iterator it;
        for(it = index.begin(); it != index.end(); it++)
        {
            const std::string& name = *it;
            if(name != kSeparatorStr)
            {
                std::map<std::string, NodePtr>::const_iterator it2 = map.find(name);
                NodePtr node = it2->second;
                n = node->renumberItems(n);
            }
        }
    }
    return n;
}

// get size, counting only leaf nodes that are not separators.
int MLMenu::Node::getSize(int n)
{
    bool isLeaf = (index.size() == 0);
    if(isLeaf)
    {
        n++;
    }
    else
    {
        std::map<std::string, NodePtr>::const_iterator it;
        for(it = map.begin(); it != map.end(); it++)
        {
            std::string name = it->first;
            if(name != kSeparatorStr)
            {
                NodePtr node = it->second;
                n = node->getSize(n);
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
        if(itemNumber >= nameVec.size())
        {
            nameVec.resize(itemNumber + 1);
        }
        nameVec[itemNumber] = path;
    }
    else
    {
        std::list<std::string>::const_iterator it;
        for(it = index.begin(); it != index.end(); it++)
        {
            const std::string& name = *it;
            std::map<std::string, NodePtr>::const_iterator it2 = map.find(name);
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
            pMenu->addItem(itemNumber, stripExtension(getShortName(name)));
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
            std::map<std::string, NodePtr>::const_iterator it2 = map.find(name);
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
            std::map<std::string, NodePtr>::const_iterator it2 = map.find(name);
            const std::string& nodeName = it2->first;
            NodePtr node = it2->second;
            node->addToJuceMenu(nodeName, subMenu, false);
        }
        
        // add submenu to parent
        pMenu->addSubMenu(name, *subMenu);
    }
}

// --------------------------------------------------------------------------------
#pragma mark MLMenu

MLMenu::MLMenu() :
    mName(""),
    mRoot(new Node())
{
}

MLMenu::MLMenu(const MLSymbol name) :
    mName(name),
    mRoot(new Node())
{
}

MLMenu::~MLMenu()
{
}

void MLMenu::addItem(const std::string& name, bool e)
{
    NodePtr n(new Node());
    n->enabled = e;
    mRoot->map[name] = n;
    mRoot->index.push_back(name);
}

MLMenu::NodePtr MLMenu::getItem(const std::string& name)
{
    NodePtr n;
    std::map<std::string, NodePtr>::const_iterator it = mRoot->map.find(name);
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
    std::map<std::string, NodePtr>::const_iterator it;
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
    
    /*
    // DEBUG
    debug() << "fullnames by index: \n";
    int size = mFullNamesByIndex.size();
    for(int i = 0; i < size; ++i)
    {
        debug() << " #" << i << ": " << mFullNamesByIndex[i] << "\n";
    }*/
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

const std::string& MLMenu::getItemFullName(int idx)
{
    int items = getSize();
    // items are 1-indexed
	if(within(idx, 0, items + 1))
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

