
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "JuceHeader.h"

#include "MLDSP.h"
#include "MLLookAndFeel.h"
#include "MLSymbol.h"

// adapter to Juce menu
class MLMenu
{
public:
	MLMenu();
	~MLMenu();
	
	void addItem(const std::string& pItemStr);
	void addItems(std::list<std::string>& pItemList);

	void clear();
	const std::string& getItemString(int idx);
	PopupMenu& getJuceMenu();
	
	MLSymbol mName;
	PopupMenu mJuceMenu;
	int mNumItems;
	std::vector<std::string> mItems;
};


typedef std::tr1::shared_ptr<MLMenu> MLMenuPtr;
