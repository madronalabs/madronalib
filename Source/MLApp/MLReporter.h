
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef __ML_REPORTER_H
#define __ML_REPORTER_H

#include "MLModel.h"
#include "MLWidget.h"
#include <map>
#include "pa_ringbuffer.h"

#pragma mark property viewing

class MLPropertyView
{
	friend class MLReporter;
public:
	MLPropertyView(MLWidget* w, MLSymbol attr);
	~MLPropertyView();
	void view(const MLProperty& v) const;
	
private:
	MLWidget* mpWidget;
	MLSymbol mAttr;
};

typedef std::shared_ptr<MLPropertyView> MLPropertyViewPtr;
typedef std::list<MLPropertyViewPtr> MLPropertyViewList;
typedef std::map<MLSymbol, MLPropertyViewList> MLPropertyViewListMap;

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
	void addPropertyViewToMap(MLSymbol p, MLWidget* w, MLSymbol attr);
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
		void doPropertyChangeAction(MLSymbol property, const MLProperty& newVal);

	private:
		MLReporter* mpOwnerReporter;
	};
	
	// TODO write a Timer class. juce::Timer is the only reason Juce is needed here. temporary.
	class ReporterTimer : public juce::Timer
	{
	public:
		ReporterTimer(MLReporter*);
		~ReporterTimer();
		void timerCallback();
	private:
		MLReporter* mpOwnerReporter;
	};
	
	void enqueuePropertyChange(MLSymbol property, const MLProperty& newVal);

	std::vector<MLPropertyListenerPtr> pListeners;
	MLPropertySet mCurrentProperties;
	std::vector<MLSymbol> mChangeData;
	PaUtilRingBuffer mChangeQueue;
	std::unique_ptr<ReporterTimer> mpTimer;
};

#endif // __ML_REPORTER_H