
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef __Soundplane__MLMenu__
#define __Soundplane__MLMenu__

#include "JuceHeader.h"
#include "MLDSP.h"
#include "MLLookAndFeel.h"
#include "MLSymbol.h"

class MLMenu;
typedef std::tr1::shared_ptr<MLMenu> MLMenuPtr;
typedef std::map<MLSymbol, MLMenuPtr> MLMenuMapT;
typedef std::tr1::shared_ptr<juce::PopupMenu> JuceMenuPtr;

// adapter to Juce menu
class MLMenu
{
public:
    
    class Node;
    typedef std::tr1::shared_ptr<Node> NodePtr;
    class Node
    {
    public:
        Node() : itemNumber(0){}
        ~Node(){}
        void clear() { map.clear(); }
        void dump(int level = 0);
        //void addItemsToVector(std::vector<NodePtr>& v);
        int renumberItems(int n = 0);
        void addToJuceMenu(const std::string& name, JuceMenuPtr pMenu, bool root = true);
        
        std::map<std::string, NodePtr> map;
        std::list<std::string> index;
        bool enabled;
        int itemNumber;
        JuceMenuPtr subMenu;
    };

	MLMenu();
	MLMenu(const MLSymbol name);
	~MLMenu();
    
	void clear();
	//void addItem(const char * name, bool enabled = true);
	void addItem(const std::string& name, bool enabled = true);
	NodePtr getItem(const std::string& name);
	void addItems(const std::vector<std::string>& items);
	void addSubMenu(MLMenuPtr m, const std::string& name);
	void appendMenu(MLMenuPtr m);
	void setItemOffset(int f) { mItemOffset = f; }
    void renumber() { mItems->renumberItems(); }
    
	void addSeparator();	
	MLSymbol getName() { return mName; }
	int getNumItems() const { return mItemsByIndex.size(); }
	const std::string getItemString(int idx);
    
    // build a Juce menu on the fly and return it
	JuceMenuPtr getJuceMenu();	

	void setInstigator(MLSymbol n) {mInstigatorName = n;}
	MLSymbol getInstigator() const {return mInstigatorName;}
    
    void dump();

protected:
	//const std::vector<std::string>& getItemVector() { return mItems; }
	//const Node& getItemVector() { return mItems; }

private:	
	MLSymbol mName;
	MLSymbol mInstigatorName; // name of Widget that triggered us
	int mItemOffset; // offset for returned item values, useful for submenus
    
    NodePtr mItems;
    std::vector<NodePtr> mItemsByIndex;
};


#endif // __Soundplane__MLMenu__