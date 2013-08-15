
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "JuceHeader.h"
#include "MLDSP.h"
#include "MLLookAndFeel.h"
#include "MLSymbol.h"

class MLMenu;
typedef std::tr1::shared_ptr<MLMenu> MLMenuPtr;
typedef std::map<MLSymbol, MLMenuPtr> MLMenuMapT;

// adapter to Juce menu
class MLMenu
{
public:
	MLMenu();
	MLMenu(const MLSymbol name);
	~MLMenu();
	
	void clear();
	void addItem(const char * name, bool enabled = true);
	void addItem(const std::string& name, bool enabled = true);	
	void addItems(const std::vector<std::string>& items);
	void addSubMenu(MLMenuPtr m, const char* name, bool enabled = true);
	void setItemOffset(int f) { mItemOffset = f; }
	void addSeparator();	
	MLSymbol getName() { return mName; }
	int getNumItems() const { return mNumItems; }
	const std::string& getItemString(int idx);
	PopupMenu& getJuceMenu();	

	void setInstigator(MLSymbol n) {mInstigatorName = n;}
	MLSymbol getInstigator() const {return mInstigatorName;} 

protected:
	const std::vector<std::string>& getItemVector() { return mItems; }

private:	
	MLSymbol mName;
	MLSymbol mInstigatorName; // name of Widget that triggered us
	PopupMenu mJuceMenu;		
	int mItemOffset; // offset for returned item values, useful for submenus
	int mNumItems;
	std::vector<std::string> mItems;
	std::vector<MLMenuPtr> mSubMenus;
};

