
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

/*
// recursively add all the items in the node to the argument vector.
void MLMenu::Node::addItemsToVector(std::vector<NodePtr>& v)
{
    std::list<std::string>::const_iterator it;
	for(it = index.begin(); it != index.end(); it++)
	{
        const std::string& name = *it;
        std::map<std::string, NodePtr>::const_iterator it = map.find(name);
        NodePtr node = it->second;
		v.push_back(node);
        node->addItemsToVector(v);
	}
}
*/

int MLMenu::Node::renumberItems(int n)
{
    int c = n + 1;
    itemNumber = c;
    std::list<std::string>::const_iterator it;
	for(it = index.begin(); it != index.end(); it++)
	{
        const std::string& name = *it;
        std::map<std::string, NodePtr>::const_iterator it2 = map.find(name);
        NodePtr node = it2->second;
        //node->itemNumber = c;
        c = node->renumberItems(c);
	}
    return c;
}

void MLMenu::Node::addToJuceMenu(const std::string& name, JuceMenuPtr pMenu, bool root)
{
    int size = index.size();
    bool isLeaf = (size == 0);
    if(isLeaf)
    {
        pMenu->addItem(itemNumber, name);
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
    mItemOffset(0),
    mItems(new Node())
{
}

MLMenu::MLMenu(const MLSymbol name) :
    mName(name),
    mItemOffset(0),
    mItems(new Node())
{
}

MLMenu::~MLMenu()
{
}

void MLMenu::addItem(const std::string& name, bool e)
{
    NodePtr n(new Node());
    n->enabled = e;
    mItems->map[name] = n;
    mItems->index.push_back(name);
    mItemsByIndex.push_back(n);
}

MLMenu::NodePtr MLMenu::getItem(const std::string& name)
{
    NodePtr n;
    std::map<std::string, NodePtr>::const_iterator it = mItems->map.find(name);
    if(it != mItems->map.end())
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
    mItems->map[name] = m->mItems;
    mItems->index.push_back(name);
    
    // add nodes in sub menu to flat index
    //m->mItems->addItemsToVector(mItemsByIndex);
}

void MLMenu::appendMenu(MLMenuPtr m)
{
    std::map<std::string, NodePtr>::const_iterator it;

    debug() << "appending menu " << m->getName();
    
    // add each element of other menu in turn to our item map
	for(it = m->mItems->map.begin(); it != m->mItems->map.end(); it++)
	{
        const std::string& name = it->first;
        NodePtr node = it->second;
        mItems->map[name] = node;
        mItems->index.push_back(name);
       
        debug() << "    appending item " << name << "\n";
	}
}

void MLMenu::addSeparator()
{
    addItem(kSeparatorStr);
}

void MLMenu::clear()
{
	mItems->clear();
    mItemsByIndex.clear();
}

const std::string MLMenu::getItemString(int idx)
{
    int items = getNumItems();
	if(within(idx, 0, items))
	{
	//	return mItems[idx];
        return std::string("OK"); // temp TODO build string from path
	}
	return kNullStr;
}

JuceMenuPtr MLMenu::getJuceMenu()
{
    renumber();
    JuceMenuPtr jm(new PopupMenu());
    mItems->addToJuceMenu(mName.getString(), jm);
    return jm;
}

void MLMenu::dump()
{
    debug() << " dump of menu " << mName << ":\n";
    mItems->dump();
    debug() << "\n\n";
}

