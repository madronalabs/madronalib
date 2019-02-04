
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef __ML_REPORTER_H
#define __ML_REPORTER_H

#include "MLModel.h"
#include "MLWidget.h"
#include <map>
#include "MLQueue.h"

using namespace ml;

#pragma mark property viewing

class MLPropertyView
{
	friend class MLReporter;
public:
	MLPropertyView(MLWidget* w, ml::Symbol attr);
	~MLPropertyView();
	void view(const MLProperty& v) const;
	
private:
	MLWidget* mpWidget;
	ml::Symbol mAttr;
};

typedef std::shared_ptr<MLPropertyView> MLPropertyViewPtr;
typedef std::list<MLPropertyViewPtr> MLPropertyViewList;
typedef std::map<ml::Symbol, MLPropertyViewList> MLPropertyViewListMap;

#pragma mark MLReporter 

// Reporter listens to one or more property sets and reports their changing properties by setting
// properties of Widgets. Properties may contain float, string or signal values.
//
class MLReporter
{
public:
	MLReporter();
    ~MLReporter();
	
	void listenTo(MLPropertySet* p);
	void fetchChangedProperties();
	void fetchAllProperties();
	void addPropertyViewToMap(ml::Symbol p, MLWidget* w, ml::Symbol attr);
	void viewProperties();

protected:
	MLPropertyViewListMap mPropertyViewsMap;
	
private:
	
	// our subclass of MLPropertyListener that forwards actions to us.
	class PropertyListener : public MLPropertyListener
	{
	public:
		PropertyListener(MLReporter* p, MLPropertySet* set) :
			MLPropertyListener(set),
			mpOwnerReporter(p)
			{}
		~PropertyListener()  {}
		void doPropertyChangeAction(ml::Symbol property, const MLProperty& newVal);

	private:
		MLReporter* mpOwnerReporter;
	};
	
	void enqueuePropertyChange(ml::Symbol property, const MLProperty& newVal);

	std::vector<MLPropertyListenerPtr> pListeners;
	MLPropertySet mCurrentProperties;
	
	std::unique_ptr< Queue< Symbol > > mChangeQueue;
	
	ml::Timer mTimer;
};

#endif // __ML_REPORTER_H
