
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef __ML_REPORTER_H
#define __ML_REPORTER_H

#include "MLModel.h"
#include "MLWidget.h"
#include <map>

// --------------------------------------------------------------------------------
#pragma mark property viewing

class MLPropertyView
{
public:
	MLPropertyView(MLWidget* w, MLSymbol attr);
	~MLPropertyView();
	void view(const MLProperty& v) const;
	
private:
	MLWidget* mpWidget;
	MLSymbol mAttr;
};

typedef std::tr1::shared_ptr<MLPropertyView> MLPropertyViewPtr;
typedef std::list<MLPropertyViewPtr> MLPropertyViewList;
typedef std::map<MLSymbol, MLPropertyViewList> MLPropertyViewListMap;

// --------------------------------------------------------------------------------
#pragma mark MLReporter 

// Reporter listens to a Model and reports its changing properties by setting
// Attributes of Widgets. Properties may contain float, string or signal values.
//
class MLReporter :
	public MLPropertyListener
{
public:
	MLReporter(MLPropertySet* m);
    ~MLReporter();

	// parameter viewing
	void addPropertyViewToMap(MLSymbol p, MLWidget* w, MLSymbol attr);
	void viewAllProperties();
	void viewAllChangedProperties();

	// MLPropertyListener interface
	void doPropertyChangeAction(MLSymbol param, const MLProperty& newVal);

protected:

	MLPropertyViewListMap mPropertyViewsMap;
};

#endif // __ML_REPORTER_H